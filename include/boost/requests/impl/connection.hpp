//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_REQUESTS_IMPL_CONNECTION_HPP
#define BOOST_REQUESTS_IMPL_CONNECTION_HPP

#include <boost/requests/stream.hpp>
#include <boost/requests/connection.hpp>
#include <boost/requests/detail/async_coroutine.hpp>
#include <boost/requests/detail/config.hpp>
#include <boost/requests/detail/defaulted.hpp>
#include <boost/requests/detail/ssl.hpp>
#include <boost/requests/fields/location.hpp>
#include <boost/requests/request_settings.hpp>
#include <boost/requests/detail/tracker.hpp>

#include <boost/asem/lock_guard.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/beast/core/buffer_ref.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <boost/core/exchange.hpp>
#include <boost/optional.hpp>
#include <boost/smart_ptr/allocate_unique.hpp>
#include <boost/url/grammar/ci_string.hpp>


namespace boost {
namespace requests {

struct connection::async_connect_op : asio::coroutine
{
  using executor_type = asio::any_io_executor;
  executor_type get_executor() {return this_->get_executor(); }

  connection * this_;
  endpoint_type ep;

  asio::coroutine inner_coro;

  async_connect_op(connection * this_, endpoint_type ep) : this_(this_), ep(ep) {}

  using mutex_type = detail::basic_mutex<executor_type>;
  using lock_type = asem::lock_guard<detail::basic_mutex<executor_type>>;

  lock_type read_lock, write_lock;

  using completion_signature_type = void(system::error_code);
  using step_signature_type       = void(system::error_code);

  BOOST_REQUESTS_DECL
  void resume(requests::detail::co_token_t<step_signature_type> self,
              system::error_code & ec);
};

template<BOOST_ASIO_COMPLETION_TOKEN_FOR(void (system::error_code)) CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void (system::error_code))
connection::async_connect(endpoint_type ep, CompletionToken && completion_token)
{

  return detail::co_run<async_connect_op>(
      std::forward<CompletionToken>(completion_token), this, ep);
}

struct connection::async_close_op : asio::coroutine
{
  connection * this_;
  detail::tracker t{this_->ongoing_requests_};

  using executor_type = asio::any_io_executor;
  executor_type get_executor() const {return this_->get_executor();}

  using lock_type = asem::lock_guard<asem::mt::mutex>;
  lock_type read_lock, write_lock;


  async_close_op(connection * this_) : this_(this_) {}

  using completion_signature_type = void(system::error_code);
  using step_signature_type       = void(system::error_code);

  BOOST_REQUESTS_DECL
  void resume(requests::detail::co_token_t<step_signature_type> self,
              system::error_code & ec);
};

template<BOOST_ASIO_COMPLETION_TOKEN_FOR(void (system::error_code)) CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void (system::error_code))
connection::async_close(CompletionToken && completion_token)
{
  return detail::co_run<async_close_op>(
      std::forward<CompletionToken>(completion_token), this);
}

namespace detail
{

struct drop_size_t
{
  auto operator()(system::error_code ec, std::size_t n) -> asio::deferred_values<system::error_code>
  {
    return asio::deferred.values(std::move(ec));
  }
};
static asio::deferred_function<drop_size_t> drop_size()
{
  return asio::deferred(drop_size_t{});
}

}

namespace detail
{

BOOST_REQUESTS_DECL bool check_endpoint(
    urls::url_view path,
    const asio::ip::tcp::endpoint  & ep,
    core::string_view host,
    bool has_ssl,
    system::error_code & ec);

BOOST_REQUESTS_DECL bool check_endpoint(
    urls::url_view path,
    const asio::local::stream_protocol::endpoint & ep,
    core::string_view host,
    bool,
    system::error_code & ec);

BOOST_REQUESTS_DECL bool check_endpoint(
    urls::url_view path,
    const asio::generic::stream_protocol::endpoint  & ep,
    core::string_view host,
    bool has_ssl,
    system::error_code & ec);

}

template<typename RequestBody>
auto connection::ropen(beast::http::verb method,
           urls::url_view path,
           RequestBody && body,
           request_settings req) -> stream
{
  system::error_code ec;
  auto res = ropen(method, path, std::forward<RequestBody>(body), std::move(req), ec);
  if (ec)
    throw_exception(system::system_error(ec));
  return res;
}

template<typename RequestBody>
auto connection::ropen(
    beast::http::verb method,
    urls::url_view path,
    RequestBody && body,
    request_settings req,
    system::error_code & ec) -> stream
{
  const auto is_secure = use_ssl_;

  if (!detail::check_endpoint(path, endpoint_, host_, use_ssl_, ec))
    return stream{get_executor(), this};

  if (((endpoint_.protocol() == asio::ip::tcp::v4())
    || (endpoint_.protocol() == asio::ip::tcp::v6()))
      && !is_secure && req.opts.enforce_tls)
  {
    BOOST_REQUESTS_ASSIGN_EC(ec, error::insecure);
    return stream{get_executor(), this};
  }

  auto src = requests::make_source(std::forward<RequestBody>(body));
  return ropen(method, path.encoded_target(), req.fields, src, std::move(req.opts), req.jar, ec);
}


struct connection::async_ropen_op
    : boost::asio::coroutine
{
  using executor_type = asio::any_io_executor;
  executor_type get_executor() {return this_->get_executor(); }

  using lock_type = asem::lock_guard<asem::mt::mutex>;

  connection * this_;
  optional<stream> str;

  detail::tracker t{this_->ongoing_requests_};

  beast::http::verb method;
  urls::pct_string_view path;
  http::fields & headers;
  source & src;
  request_options opts;
  cookie_jar * jar{nullptr};
  response_base::buffer_type buf{headers.get_allocator()};

  lock_type lock;
  boost::optional<lock_type> alock;

  response_base::history_type history;
  system::error_code ec_;

  async_ropen_op(connection * this_,
                 beast::http::verb method,
                 urls::pct_string_view path,
                 http::fields & headers,
                 source & src,
                 request_options opts,
                 cookie_jar * jar)
      : this_(this_), method(method), path(path), headers(headers), src(src), opts(std::move(opts)), jar(jar)
  {
  }


  async_ropen_op(connection * this_,
                 beast::http::verb method,
                 urls::url_view path,
                 http::fields & headers,
                 source & src,
                 request_options opt,
                 cookie_jar * jar)
      : this_(this_), method(method), path(path.encoded_resource()),
        headers(headers), src(src), opts(std::move(opt)), jar(jar)
  {
    detail::check_endpoint(path, this_->endpoint(), this_->host(), this_->use_ssl_, ec_);
  }

  using completion_signature_type = void(system::error_code, stream);
  using step_signature_type       = void(system::error_code, std::size_t);

  BOOST_REQUESTS_DECL
  auto resume(requests::detail::co_token_t<step_signature_type> self,
              system::error_code & ec, std::size_t res_ = 0u) -> stream;
};

template<typename RequestSource>
struct connection::async_ropen_op_body_base
{
  RequestSource source_impl;
  http::fields headers;

  template<typename RequestBody>
  async_ropen_op_body_base(RequestBody && body, http::fields headers)
      : source_impl(requests::make_source(std::forward<RequestBody>(body))), headers(std::move(headers))
  {
  }
};

template<typename RequestSource>
struct connection::async_ropen_op_body : async_ropen_op_body_base<RequestSource>, async_ropen_op
{
  template<typename RequestBody>
  async_ropen_op_body(
      connection * this_,
      beast::http::verb method,
      urls::url_view path,
      RequestBody && body,
      request_settings req)
      : async_ropen_op_body_base<RequestSource>{std::forward<RequestBody>(body), std::move(req.fields)},
        async_ropen_op(this_, method, path, async_ropen_op_body_base<RequestSource>::headers,
                       this->source_impl, std::move(req.opts), req.jar)
  {}
};


template<typename RequestBody, typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void (boost::system::error_code,
                                                         stream))
connection::async_ropen(
    beast::http::verb method,
    urls::url_view path,
    RequestBody && body,
    request_settings req,
    CompletionToken && completion_token)
{
  using rp = async_ropen_op_body<std::decay_t<decltype(make_source(std::forward<RequestBody>(body)))>>;
  return detail::co_run<rp>(
      std::forward<CompletionToken>(completion_token),
      this, method, path, std::forward<RequestBody>(body),
      std::move(req));
}

template<typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                   void (boost::system::error_code,
                                        stream))
connection::async_ropen(beast::http::verb method,
                        urls::pct_string_view path,
                        http::fields & headers,
                        source & src,
                        request_options opt,
                        cookie_jar * jar,
                        CompletionToken && completion_token)
{
  return detail::co_run<async_ropen_op>(std::forward<CompletionToken>(completion_token),
                                        this, method, path, std::ref(headers), std::ref(src), std::move(opt), jar);
}

}
}


#if defined(BOOST_REQUESTS_HEADER_ONLY)
#include <boost/requests/impl/connection.ipp>
#endif

#endif // BOOST_REQUESTS_IMPL_CONNECTION_HPP

