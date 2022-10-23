// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_REQUESTS_COOKIES_GRAMMAR_COOKIE_OCTET_HPP
#define BOOST_REQUESTS_COOKIES_GRAMMAR_COOKIE_OCTET_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/grammar/detail/charset.hpp>

namespace boost {
namespace requests {
namespace grammar
{

/** The set of cookie octets

    @par Example
    Character sets are used with rules and the
    functions @ref find_if and @ref find_if_not.
    @code
    result< string_view > rv = parse( "2122", token_rule( cookie_octet ) );
    @endcode

    @par BNF
    @code
     cookie-octet      = %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E
                           ; US-ASCII characters excluding CTLs,
                           ; whitespace DQUOTE, comma, semicolon,
                           ; and backslash
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc6265#section-4.1"
        >4.1.1.  Syntax  (rfc6265)</a>

    @see
        @ref find_if,
        @ref find_if_not,
        @ref parse,
        @ref token_rule.
*/
#ifdef BOOST_REQUESTS_DOCS
constexpr __implementation_defined__ cookie_octets;
#else

struct cookie_octets_t
{
    constexpr
    bool
    operator()(char c) const noexcept
    {
        return c == '\x21'
           || (c >= '\x23' && c <= '\x2B')
           || (c >= '\x2D' && c <= '\x3A')
           || (c >= '\x3C' && c <= '\x5B')
           || (c >= '\x5D' && c <= '\x7E');
    }

#ifdef BOOST_URL_USE_SSE2

    char const *
    find_if(
            char const *first,
            char const *last) const noexcept
    {
        return urls::grammar::detail::find_if_pred(
                *this, first, last);
    }

    char const *
    find_if_not(
            char const *first,
            char const *last) const noexcept
    {
        return urls::grammar::detail::find_if_not_pred(
                *this, first, last);
    }

#endif
};

constexpr cookie_octets_t cookie_octets{};
#endif

} // grammar
} // requests
} // boost

#endif //BOOST_REQUESTS_COOKIES_GRAMMAR_COOKIE_OCTET_HPP
