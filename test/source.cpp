// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/requests/json.hpp>
#include <boost/requests/source.hpp>
#include <boost/requests/request_settings.hpp>

#include "doctest.h"
#include "string_maker.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/connect_pipe.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <thread>
#include <boost/beast/http/read.hpp>

using namespace boost;

TEST_SUITE_BEGIN("source");

TEST_CASE("sync")
{
  asio::io_context ctx;
  asio::readable_pipe rp{ctx};
  asio::writable_pipe wp{ctx};
  asio::connect_pipe(rp, wp);

  std::thread thr{
      [&]{
        system::error_code ec;
        auto sp = tag_invoke(requests::make_source_tag{}, json::value{"foobaria"});
        requests::http::fields hd;
        write_request(wp,
                      requests::http::verb::post, "/test", hd,
                      sp,
                      ec);
        CHECK(ec == system::error_code{});
        hd.clear();
        auto ep = requests::make_source(requests::empty());
        write_request(wp,
                      requests::http::verb::get, "/test2", hd,
                      ep,
                      ec);
        CHECK(ec == system::error_code{});
      }};


  requests::http::request<beast::http::string_body> req;
  beast::flat_buffer buf;
  system::error_code ec;
  beast::http::read(rp, buf, req, ec);
  CHECK(ec == system::error_code{});
  CHECK(req.method() == beast::http::verb::post);
  CHECK(req.target() == "/test");
  CHECK(req.at(boost::beast::http::field::content_type) == "application/json");
  CHECK(json::parse(req.body()) == json::value{"foobaria"});


  requests::http::request<beast::http::empty_body> re2;
  beast::http::read(rp, buf, re2, ec);
  CHECK(ec == system::error_code{});
  CHECK(re2.method() == beast::http::verb::get);
  CHECK(re2.target() == "/test2");
  CHECK(re2.count(boost::beast::http::field::content_type) == 0);

  thr.join();
}

asio::awaitable<void> async_impl()
{
  asio::readable_pipe rp{co_await asio::this_coro::executor};
  asio::writable_pipe wp{co_await asio::this_coro::executor};
  auto sp = tag_invoke(requests::make_source_tag{}, json::string("foobaria"));
  auto ep = tag_invoke(requests::make_source_tag{}, requests::empty());
  asio::connect_pipe(rp, wp);

  co_await asio::co_spawn(
    co_await asio::this_coro::executor,
    [&]() -> asio::awaitable<void>
    {
      requests::http::fields hd;

      co_await async_write_request(wp, requests::http::verb::post, "/test", hd, sp, asio::use_awaitable);
      hd.clear();
      co_await async_write_request(wp, requests::http::verb::get, "/test2", hd, ep, asio::use_awaitable);
    },
    asio::use_awaitable);

  requests::http::request<beast::http::string_body> req;
  beast::flat_buffer buf;
  system::error_code ec;
  co_await beast::http::async_read(rp, buf, req, asio::use_awaitable);
  CHECK(req.method() == beast::http::verb::post);
  CHECK(req.target() == "/test");
  CHECK(req.at(boost::beast::http::field::content_type) == "application/json");
  CHECK(json::parse(req.body()) == "foobaria");

  requests::http::request<beast::http::empty_body> re2;
  co_await beast::http::async_read(rp, buf, re2, asio::use_awaitable);
  CHECK(ec == system::error_code{});
  CHECK(re2.method() == beast::http::verb::get);
  CHECK(re2.target() == "/test2");
  CHECK(re2.count(boost::beast::http::field::content_type) == 0);

  co_return ;
}

TEST_CASE("async")
{
  asio::io_context ctx;
  asio::co_spawn(ctx, async_impl(), [&](std::exception_ptr e){CHECK(e == nullptr);});
  ctx.run();
}


TEST_SUITE_END();