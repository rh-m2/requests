//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_REQUESTS_IMPL_STREAM_HPP
#define BOOST_REQUESTS_IMPL_STREAM_HPP

#include <boost/asio/read_until.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/beast/core/buffer_ref.hpp>
#include <boost/requests/keep_alive.hpp>
#include <boost/requests/stream.hpp>

#include <boost/asio/yield.hpp>

namespace boost
{
namespace requests
{

template<typename MutableBuffer>
std::size_t stream::read_some(const MutableBuffer & buffer, system::error_code & ec)
{
  if (!parser_)
  {
    BOOST_REQUESTS_ASSIGN_EC(ec, asio::error::not_connected);
    return 0u;
  }
  else if (!parser_->get().body().more)
  {
    BOOST_REQUESTS_ASSIGN_EC(ec, asio::error::eof);
    return 0u;
  }

  auto itr = boost::asio::buffer_sequence_begin(buffer);
  const auto end = boost::asio::buffer_sequence_end(buffer);
  if (itr == end)
    return 0u;

  parser_->get().body().data = itr->data();
  parser_->get().body().size = itr->size();

  auto res = impl_->do_read_some_(*parser_, ec);

  if (!parser_->is_done())
  {
    parser_->get().body().more = true;
    if (ec == beast::http::error::need_buffer)
      ec = {};
  }
  else
  {
    parser_->get().body().more = false;
    bool should_close = interpret_keep_alive_response(impl_->keep_alive_set_, parser_->get(), ec);
    if (should_close)
    {
      boost::system::error_code ec_;
      impl_->do_close_(ec_);
      return res;
    }
  }

  return res;
};


template<typename DynamicBuffer>
std::size_t stream::read(DynamicBuffer & buffer, system::error_code & ec)
{
  if (!parser_)
  {
    BOOST_REQUESTS_ASSIGN_EC(ec, asio::error::not_connected);
    return 0u;
  }
  else if (!parser_->get().body().more)
  {
    BOOST_REQUESTS_ASSIGN_EC(ec, asio::error::eof);
    return 0u;
  }

  std::size_t res = 0u;
  while (!ec && !parser_->is_done())
  {
    auto n = read_some(buffer.prepare(parser_->content_length_remaining().value_or(BOOST_REQUESTS_CHUNK_SIZE)), ec);
    buffer.commit(n);
    res += n;
  }

  if (!parser_->is_done())
    ec = beast::http::error::need_buffer;
  else
  {
    parser_->get().body().more = false;
    bool should_close = interpret_keep_alive_response(impl_->keep_alive_set_, parser_->get(), ec);
    if (should_close)
    {
      boost::system::error_code ec_;
      impl_->do_close_(ec_);
      return res;
    }
  }
  return res;
}

template<typename DynamicBuffer>
struct stream::async_read_op : asio::coroutine
{
  using executor_type = asio::any_io_executor;
  executor_type get_executor() {return this_->get_executor(); }

  stream * this_;
  DynamicBuffer & buffer;

  template<typename DynamicBuffer_>
  async_read_op(stream * this_, DynamicBuffer_ && buffer) : this_(this_), buffer(buffer)
  {
  }

  using lock_type = asem::lock_guard<detail::basic_mutex<executor_type>>;
  lock_type lock;
  system::error_code ec_;
  std::size_t res = 0u;


  using completion_signature_type = void(system::error_code, std::size_t);
  using step_signature_type       = void(system::error_code, std::size_t);

  std::size_t resume(requests::detail::co_token_t<step_signature_type> self,
                     system::error_code & ec, std::size_t n = 0u)
  {
    reenter(this)
    {
      if (!this_->parser_)
        BOOST_REQUESTS_ASSIGN_EC(ec, asio::error::not_connected)
      else if (!this_->parser_->get().body().more)
        BOOST_REQUESTS_ASSIGN_EC(ec, asio::error::eof)

      if (ec)
        break;

      if (this_->parser_->is_done())
        break;

      while (!ec && !this_->parser_->is_done())
      {
        yield this_->async_read_some(
            buffer.prepare(this_->parser_->content_length_remaining().value_or(BOOST_REQUESTS_CHUNK_SIZE)),
            std::move(self));
        buffer.commit(n);
        res += n;
      }

      if (!this_->parser_->is_done() && !ec)
        BOOST_REQUESTS_ASSIGN_EC(ec, beast::http::error::need_buffer)
      else
      {
        this_->parser_->get().body().more = false;
        if (interpret_keep_alive_response(this_->impl_->keep_alive_set_, this_->parser_->get(), ec))
        {
          std::swap(ec, ec_);
          yield this_->impl_->do_async_close_(std::move(self));
          std::swap(ec, ec_);
        }
      }

      return res;
    }
    return 0u;
  }

};


template<
    typename DynamicBuffer,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (system::error_code, std::size_t)) CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void (system::error_code, std::size_t))
stream::async_read(
    DynamicBuffer & buffers,
    CompletionToken && token)
{
  return detail::co_run<async_read_op<DynamicBuffer>>(std::forward<CompletionToken>(token), this, buffers);
}

struct stream::async_read_some_op : asio::coroutine
{
  using executor_type = asio::any_io_executor;
  executor_type get_executor() {return this_->get_executor(); }

  stream * this_;
  asio::mutable_buffer buffer;

  template<typename MutableBufferSequence>
  async_read_some_op(stream * this_, const MutableBufferSequence & buffer) : this_(this_)
  {
    auto itr = boost::asio::buffer_sequence_begin(buffer);
    const auto end = boost::asio::buffer_sequence_end(buffer);

    while (itr != end)
    {
      if (itr->size() != 0u)
      {
        this->buffer = *itr;
        break;
      }
    }
  }

  using lock_type = asem::lock_guard<detail::basic_mutex<executor_type>>;
  lock_type lock;
  system::error_code ec_;


  using completion_signature_type = void(system::error_code, std::size_t);
  using step_signature_type       = void(system::error_code, std::size_t);

  BOOST_REQUESTS_DECL
  std::size_t resume(requests::detail::co_token_t<step_signature_type> self,
                     system::error_code & ec, std::size_t res = 0u);

};

template<
    typename MutableBufferSequence,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (system::error_code, std::size_t)) CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void (system::error_code, std::size_t))
stream::async_read_some(
    const MutableBufferSequence & buffers,
    CompletionToken && token)
{
  return detail::co_run<async_read_some_op>(std::forward<CompletionToken>(token), this, buffers);
}


struct stream::async_dump_op : asio::coroutine
{
  using executor_type = asio::any_io_executor;
  executor_type get_executor() {return this_->get_executor(); }

  stream * this_;
  using mutex_type = detail::basic_mutex<executor_type>;

  char buffer[BOOST_REQUESTS_CHUNK_SIZE];
  using lock_type = asem::lock_guard<detail::basic_mutex<executor_type>>;
  lock_type lock;
  system::error_code ec_;

  async_dump_op(stream * this_) : this_(this_) {}

  using completion_signature_type = void(system::error_code);
  using step_signature_type       = void(system::error_code, std::size_t);

  BOOST_REQUESTS_DECL
  void resume(requests::detail::co_token_t<step_signature_type> self,
              system::error_code ec = {}, std::size_t n = 0u);
};

template<BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code)) CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void (boost::system::error_code))
stream::async_dump(CompletionToken && token)
{
  return detail::co_run<async_dump_op>(std::forward<CompletionToken>(token), this);
}




}
}

#include <boost/asio/unyield.hpp>

#if defined(BOOST_REQUESTS_HEADER_ONLY)
#include <boost/requests/impl/stream.ipp>
#endif

#endif // BOOST_REQUESTS_IMPL_STREAM_HPP
