// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_REQUESTS_FACADE_HPP
#define BOOST_REQUESTS_FACADE_HPP

#include <boost/requests/body_traits.hpp>
#include <boost/requests/request.hpp>
#include <boost/requests/response.hpp>
#include <boost/requests/detail/variadic.hpp>
#include <boost/requests/facade.hpp>
#include <boost/requests/redirect.hpp>
#include <boost/beast/http/verb.hpp>

namespace boost {
namespace requests {

using empty = beast::http::empty_body::value_type;

/// Install all nice methods on the object
template<typename Derived, typename Target, typename Executor>
struct facade
{
  using target_view = Target;
  using executor_type = Executor;

  template<typename Allocator = std::allocator<char>>
  auto get(target_view target,
           basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::get, target, empty{}, std::move(req));
  }

  template<typename Allocator = std::allocator<char>>
  auto get(target_view target,
           basic_request<Allocator> req,
           system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::get, target, empty{}, std::move(req), ec);
  }

  template<typename Allocator = std::allocator<char>>
  auto head(target_view target,
            basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::head, target, empty{}, std::move(req));
  }

  template<typename Allocator = std::allocator<char>>
  auto head(target_view target,
            basic_request<Allocator> req,
            system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::head, target, empty{}, std::move(req), ec);
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto post(target_view target,
            RequestBody && request_body,
            basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::post, target,
                                           std::forward<RequestBody>(request_body), std::move(req));
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto post(target_view target,
            RequestBody && request_body,
            basic_request<Allocator> req,
            system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::post, target,
                                           std::forward<RequestBody>(request_body), std::move(req), ec);
  }


  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto put(target_view target,
           RequestBody && request_body,
           basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::put, target,
                                           std::forward<RequestBody>(request_body), std::move(req));
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto put(target_view target,
           RequestBody && request_body,
           basic_request<Allocator> req,
           system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::put, target,
                                           std::forward<RequestBody>(request_body), std::move(req), ec);
  }


  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto patch(target_view target,
             RequestBody && request_body,
             basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::patch, target,
                                           std::forward<RequestBody>(request_body), std::move(req));
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto patch(target_view target,
             RequestBody && request_body,
             basic_request<Allocator> req,
             system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::patch, target,
                                           std::forward<RequestBody>(request_body), std::move(req), ec);
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto delete_(target_view target,
               RequestBody && request_body,
               basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::delete_, target,
                                           std::forward<RequestBody>(request_body), std::move(req));
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto delete_(target_view target,
               RequestBody && request_body,
               basic_request<Allocator> req,
               system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::delete_, target,
                                           std::forward<RequestBody>(request_body), std::move(req), ec);
  }


  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto delete_(target_view target,
               basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::delete_, target, empty{}, std::move(req));
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto delete_(target_view target,
               basic_request<Allocator> req,
               system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::delete_, target, empty{}, std::move(req), ec);
  }


  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto connect(target_view target,
               basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::connect, target, empty{}, std::move(req));
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto connect(target_view target,
               basic_request<Allocator> req,
               system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::connect, target, empty{}, std::move(req), ec);
  }


  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto options(target_view target,
               basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::options, target, empty{}, std::move(req));
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto options(target_view target,
               basic_request<Allocator> req,
               system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::options, target, empty{}, std::move(req), ec);
  }


  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto trace(target_view target,
               basic_request<Allocator> req) -> basic_response<Allocator>
  {
    return static_cast<Derived*>(this)->request(http::verb::trace, target, empty{}, std::move(req));
  }

  template<typename RequestBody, typename Allocator = std::allocator<char>>
  auto trace(target_view target,
             basic_request<Allocator> req,
             system::error_code & ec) -> basic_response<Allocator>
  {
    auto p = static_cast<Derived*>(this);
    return static_cast<Derived*>(this)->request(http::verb::trace, target, empty{}, std::move(req), ec);
  }

  template<typename Allocator = std::allocator<char>,
            BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                 basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_get(target_view target,
            basic_request<Allocator> req,
            CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::get, target,
                                                      empty{}, std::move(req),
                                                      std::forward<CompletionToken>(completion_token));
  }

  template<typename Allocator = std::allocator<char>,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                 basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_head(target_view target,
             basic_request<Allocator> req,
             CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::head, target,
                                                       empty{}, std::move(req),
                                                       std::forward<CompletionToken>(completion_token));
  }

  template<typename Allocator = std::allocator<char>,
           typename RequestBody,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                 basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_post(target_view target,
             RequestBody && request_body,
             basic_request<Allocator> req,
             CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::post, target,
                                                      std::forward<RequestBody>(request_body), std::move(req),
                                                      std::forward<CompletionToken>(completion_token));
  }

  template<typename Allocator = std::allocator<char>,
           typename RequestBody,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                  basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_put(target_view target,
            RequestBody && request_body,
            basic_request<Allocator> req,
            CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::put, target,
                                                       std::forward<RequestBody>(request_body), std::move(req),
                                                       std::forward<CompletionToken>(completion_token));
  }

  template<typename Allocator = std::allocator<char>,
           typename RequestBody,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                  basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_patch(target_view target,
              RequestBody && request_body,
              basic_request<Allocator> req,
              CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::patch, target,
                                                       std::forward<RequestBody>(request_body), std::move(req),
                                                       std::forward<CompletionToken>(completion_token));
  }

  template<typename Allocator = std::allocator<char>,
           typename RequestBody,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                  basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_delete(target_view target,
               RequestBody && request_body,
               basic_request<Allocator> req,
               CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::delete_, target,
                                                       std::forward<RequestBody>(request_body), std::move(req),
                                                       std::forward<CompletionToken>(completion_token));
  }


  template<typename Allocator = std::allocator<char>,
           typename RequestBody,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                  basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_delete(target_view target,
               basic_request<Allocator> req,
               CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::delete_, target,
                                                      empty{}, std::move(req),
                                                      std::forward<CompletionToken>(completion_token));
  }


  template<typename Allocator = std::allocator<char>,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                 basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_connect(target_view target,
                basic_request<Allocator> req,
                CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::connect, target,
                                                       empty{}, std::move(req),
                                                       std::forward<CompletionToken>(completion_token));
  }


  template<typename Allocator = std::allocator<char>,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                 basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_options(target_view target,
                basic_request<Allocator> req,
                CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::options, target,
                                                       empty{}, std::move(req),
                                                       std::forward<CompletionToken>(completion_token));
  }


  template<typename Allocator = std::allocator<char>,
           BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
                                                 basic_response<Allocator>)) CompletionToken
                BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken,
                                     void (boost::system::error_code, basic_response<Allocator>))
  async_trace(target_view target,
              basic_request<Allocator> req,
              CompletionToken && completion_token BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type))
  {
    return static_cast<Derived*>(this)->async_request(http::verb::trace, target,
                                                       empty{}, std::move(req),
                                                       std::forward<CompletionToken>(completion_token));
  }


};


}
}

#endif //BOOST_REQUESTS_FACADE_HPP
