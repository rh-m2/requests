// Copyright (c) 2021 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/requests/download.hpp>
#include <boost/requests/json.hpp>
#include <boost/requests/method.hpp>
#include <boost/requests/session.hpp>
#include <boost/requests/form.hpp>
#include <boost/json.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "doctest.h"
#include "string_maker.hpp"

#include <boost/asio/experimental/awaitable_operators.hpp>

using namespace boost;

inline std::string httpbin()
{
  std::string url = "httpbin.org";
  if (auto p = ::getenv("BOOST_REQUEST_HTTPBIN"))
    url = p;
  return url;
}


TEST_SUITE_BEGIN("session");


struct http_maker
{
  urls::url url;
  http_maker(core::string_view target)
  {
    url = urls::parse_uri("http://" + httpbin() + std::string{target}).value();

  }

  operator urls::url_view () const
  {
    return url;
  }

  operator json::value () const
  {
    return json::string(url.buffer());
  }
};

struct https_maker
{
  urls::url url;
  https_maker(core::string_view target)
  {
    url = urls::parse_uri("https://" + httpbin() + std::string{target}).value();
  }

  operator urls::url_view () const
  {
    return url;
  }
  operator json::value () const
  {
    return json::string(url.buffer());
  }
};

TYPE_TO_STRING(http_maker);
TYPE_TO_STRING(https_maker);

TEST_CASE_TEMPLATE("sync-request", u, http_maker, https_maker)
{
  asio::io_context ctx;

  requests::session hc{ctx};
  hc.options().enforce_tls = false;
  hc.options().max_redirects = 5;

  SUBCASE("headers")
  {
    auto hdr = request(hc, beast::http::verb::get,
                          u("/headers"),
                          requests::empty{},
                          requests::headers({{"Test-Header", "it works"}}));

    auto hd = as_json(hdr).at("headers");

    CHECK(hd.at("Host")        == json::value(httpbin()));
    CHECK(hd.at("Test-Header") == json::value("it works"));
  }


  SUBCASE("get")
  {
    auto hdr = get(hc, u("/get"), requests::headers({{"Test-Header", "it works"}}));

    auto hd = as_json(hdr).at("headers");

    CHECK(hd.at("Host")        == json::value(httpbin()));
    CHECK(hd.at("Test-Header") == json::value("it works"));
  }

  SUBCASE("get-redirect")
  {
    auto hdr = get(hc, u("/redirect-to?url=%2Fget"), requests::headers({{"Test-Header", "it works"}}));

    CHECK(hdr.history.size() == 1u);
    CHECK(hdr.history.at(0u).at(beast::http::field::location) == "/get");

    auto hd = as_json(hdr).at("headers");

    CHECK(hd.at("Host")        == json::value(httpbin()));
    CHECK(hd.at("Test-Header") == json::value("it works"));
  }

  SUBCASE("too-many-redirects")
  {
    system::error_code ec;
    auto res = get(hc, u("/redirect/10"), {}, ec);
    CHECK(res.history.size() == 5);
    CHECK(res.headers.begin() == res.headers.end());
    CHECK(ec == requests::error::too_many_redirects);
  }

  SUBCASE("download")
  {
    const auto target = std::filesystem::temp_directory_path() / "requests-test.png";
    if (std::filesystem::exists(target))
      std::filesystem::remove(target);

    CHECK(!std::filesystem::exists(target));
    auto res = download(hc, u("/image"), {}, target.string());

    CHECK(std::stoull(res.headers.at(beast::http::field::content_length)) > 0u);
    CHECK(res.headers.at(beast::http::field::content_type) == "image/png");

    CHECK(std::filesystem::exists(target));
    std::error_code ec;
    std::filesystem::remove(target, ec);
  }


  SUBCASE("download-redirect")
  {

    const auto target = std::filesystem::temp_directory_path() / "requests-test.png";
    if (std::filesystem::exists(target))
      std::filesystem::remove(target);

    CHECK(!std::filesystem::exists(target));
    auto res = download(hc, u("/redirect-to?url=%2Fimage"), {}, target.string());

    CHECK(res.history.size() == 1u);
    CHECK(res.history.at(0u).at(beast::http::field::location) == "/image");

    CHECK(std::stoull(res.headers.at(beast::http::field::content_length)) > 0u);
    CHECK(res.headers.at(beast::http::field::content_type) == "image/png");

    CHECK(std::filesystem::exists(target));
    std::error_code ec;
    std::filesystem::remove(target, ec);
  }


  SUBCASE("download-too-many-redirects")
  {
    system::error_code ec;
    hc.options().max_redirects = 3;
    const auto target = std::filesystem::temp_directory_path() / "requests-test.html";
    if (std::filesystem::exists(target))
      std::filesystem::remove(target);
    auto res = download(hc, u("/redirect/10"), {}, target.string(), ec);
    CHECK(res.history.size() == 3);

    CHECK(res.headers.begin() == res.headers.end());

    CHECK(ec == requests::error::too_many_redirects);
    CHECK(!std::filesystem::exists(target));
  }

  SUBCASE("delete")
  {
    auto hdr = delete_(hc, u("/delete"), json::value{{"test-key", "test-value"}}, {});

    auto js = as_json(hdr);
    CHECK(beast::http::to_status_class(hdr.headers.result()) == beast::http::status_class::successful);
    CHECK(js.at("headers").at("Content-Type") == "application/json");
  }

  SUBCASE("patch-json")
  {
    json::value msg {{"test-key", "test-value"}};
    auto hdr = patch(hc, u("/patch"), msg, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/json");
    CHECK(js.at("json") == msg);
  }

  SUBCASE("patch-form")
  {
    auto hdr = patch(hc, u("/patch"),
                        requests::form{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}},
                        {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/x-www-form-urlencoded");
    CHECK(js.at("form") == json::value{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}});
  }

  SUBCASE("put-json")
  {
    json::value msg {{"test-key", "test-value"}};
    auto hdr = put(hc, u("/put"), msg, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/json");
    CHECK(js.at("json") == msg);
  }

  SUBCASE("put-form")
  {
    auto hdr = put(hc, u("/put"),
                      requests::form{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}},
                      {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/x-www-form-urlencoded");
    CHECK(js.at("form") == json::value{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}});
  }

  SUBCASE("post-json")
  {
    json::value msg {{"test-key", "test-value"}};
    auto hdr = post(hc, u("/post"), msg, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/json");
    CHECK(js.at("json") == msg);
  }

  SUBCASE("post-form")
  {
    auto hdr = post(hc, u("/post"),
                       requests::form{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}},
                       {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/x-www-form-urlencoded");
    CHECK(js.at("form") == json::value{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}});
  }

}


template<typename u>
asio::awaitable<void> async_http_pool_request()
{
  auto url = httpbin();
  using exec_t = typename asio::use_awaitable_t<>::executor_with_default<asio::any_io_executor>;
  requests::session::defaulted<asio::use_awaitable_t<>> hc{co_await asio::this_coro::executor};
  hc.options().enforce_tls = false;
  hc.options().max_redirects = 3;

  auto headers = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    auto hdr = co_await async_request(hc,
        beast::http::verb::get, u("/headers"),
        requests::empty{}, requests::headers({{"Test-Header", "it works"}}));

    auto hd = as_json(hdr).at("headers");

    CHECK(hd.at("Host")        == json::value(url));
    CHECK(hd.at("Test-Header") == json::value("it works"));
  };

  auto get_ = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    auto h = requests::headers({{"Test-Header", "it works"}});
    auto hdr = co_await async_get(hc, u("/get"), h);

    auto hd = as_json(hdr).at("headers");

    CHECK(hd.at("Host")        == json::value(url));
    CHECK(hd.at("Test-Header") == json::value("it works"));
  };


  auto get_redirect = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    auto h = requests::headers({{"Test-Header", "it works"}});
    auto hdr = co_await async_get(hc, u("/redirect-to?url=%2Fget"), h);

    CHECK(hdr.history.size() == 1u);
    CHECK(hdr.history.at(0u).at(beast::http::field::location) == "/get");

    auto hd = as_json(hdr).at("headers");

    CHECK(hd.at("Host")        == json::value(url));
    CHECK(hd.at("Test-Header") == json::value("it works"));
  };

  auto too_many_redirects = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    system::error_code ec;

    auto res = co_await async_get(hc, u("/redirect/10"), {},
                                     asio::redirect_error(asio::use_awaitable, ec));
    CHECK(res.history.size() == 2);
    CHECK(beast::http::to_status_class(res.headers.result()) == beast::http::status_class::redirection);
    CHECK(ec == requests::error::too_many_redirects);
  };


  auto download = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    auto pt = std::filesystem::temp_directory_path();

    const auto target = pt / "requests-test.png";
    if (std::filesystem::exists(target))
      std::filesystem::remove(target);

    CHECK(!std::filesystem::exists(target));
    auto res = co_await async_download(hc, u("/image"), {}, target.string());

    CHECK(std::stoull(res.headers.at(beast::http::field::content_length)) > 0u);
    CHECK(res.headers.at(beast::http::field::content_type) == "image/png");

    CHECK(std::filesystem::exists(target));
    std::error_code ec;
    std::filesystem::remove(target, ec);
  };


  auto download_redirect = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    const auto target = std::filesystem::temp_directory_path() / "requests-test-2.png";
    if (std::filesystem::exists(target))
      std::filesystem::remove(target);

    CHECK(!std::filesystem::exists(target));
    auto res = co_await async_download(hc, u("/redirect-to?url=%2Fimage"), {}, target.string());

    CHECK(res.history.size() == 1u);
    CHECK(res.history.at(0u).at(beast::http::field::location) == "/image");

    CHECK(std::stoull(res.headers.at(beast::http::field::content_length)) > 0u);
    CHECK(res.headers.at(beast::http::field::content_type) == "image/png");

    CHECK(std::filesystem::exists(target));
    std::error_code ec;
    std::filesystem::remove(target, ec);
  };


  auto download_too_many_redirects = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    system::error_code ec;
    const auto target = std::filesystem::temp_directory_path() / "requests-test.html";
    if (std::filesystem::exists(target))
      std::filesystem::remove(target);

    auto res = co_await async_download(hc, u("/redirect/10"), {}, target.string(),
                                          asio::redirect_error(asio::use_awaitable, ec));
    CHECK(res.history.size() == 2);

    CHECK(beast::http::to_status_class(res.headers.result()) == beast::http::status_class::redirection);

    CHECK(ec == requests::error::too_many_redirects);
    CHECK(!std::filesystem::exists(target));
  };


  auto delete_ = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    json::value v{{"test-key", "test-value"}};
    auto hdr = co_await async_delete(hc, u("/delete"), v, {});

    auto js = as_json(hdr);
    CHECK(beast::http::to_status_class(hdr.headers.result()) == beast::http::status_class::successful);
    CHECK(js.at("headers").at("Content-Type") == "application/json");
  };

  auto patch_json = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    json::value msg {{"test-key", "test-value"}};
    auto hdr = co_await async_patch(hc, u("/patch"), msg, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/json");
    CHECK(js.at("json") == msg);
  };

  auto patch_form = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    requests::form f{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}};
    auto hdr = co_await async_patch(hc, u("/patch"), f, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/x-www-form-urlencoded");
    CHECK(js.at("form") == json::value{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}});
  };

  auto put_json = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    json::value msg {{"test-key", "test-value"}};
    auto hdr = co_await async_put(hc, u("/put"), msg, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/json");
    CHECK(js.at("json") == msg);
  };

  auto put_form = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    requests::form f{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}};
    auto hdr = co_await async_put(hc, u("/put"), f, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/x-www-form-urlencoded");
    CHECK(js.at("form") == json::value{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}});
  };

  auto post_json = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    json::value msg {{"test-key", "test-value"}};

    auto hdr = co_await async_post(hc, u("/post"), msg, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/json");
    CHECK(js.at("json") == msg);
  };

  auto post_form = [](requests::session::defaulted<asio::use_awaitable_t<>> & hc, core::string_view url) -> asio::awaitable<void>
  {
    requests::form f = {{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}};
    auto hdr = co_await async_post(hc, u("/post"), f, {});

    auto js = as_json(hdr);
    CHECK(hdr.headers.result() == beast::http::status::ok);
    CHECK(js.at("headers").at("Content-Type") == "application/x-www-form-urlencoded");
    CHECK(js.at("form") == json::value{{"foo", "42"}, {"bar", "21"}, {"foo bar" , "23"}});
  };

  using namespace asio::experimental::awaitable_operators ;
  co_await (
      headers(hc, url)
      && get_(hc, url)
      && get_redirect(hc, url)
      && too_many_redirects(hc, url)
      && download(hc, url)
      && download_redirect(hc, url)
      && download_too_many_redirects(hc, url)
      && delete_(hc, url)
      && patch_json(hc, url)
      && patch_form(hc, url)
      && put_json(hc, url)
      && put_form(hc, url)
      && post_json(hc, url)
      && post_form(hc, url)
  );
  co_await asio::post(asio::use_awaitable);
}

TEST_CASE_TEMPLATE("async-session-request", M, http_maker, https_maker)
{
  auto url = httpbin();

  asio::io_context ctx;
  asio::co_spawn(ctx,
                 async_http_pool_request<M>(),
                 [](std::exception_ptr e)
                 {
                   CHECK(e == nullptr);
                 });

  ctx.run();
}

TEST_SUITE_END();