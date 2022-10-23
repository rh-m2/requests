//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_REQUESTS_IMPL_REQUEST_IPP
#define BOOST_REQUESTS_IMPL_REQUEST_IPP


#if defined(BOOST_REQUESTS_SOURCE)

#include <boost/requests/request.hpp>

namespace boost {
namespace requests {

template struct basic_request<std::allocator<char>>;
template struct basic_request<container::pmr::polymorphic_allocator<char>>;

}
}

#endif


#endif // BOOST_REQUESTS_IMPL_REQUEST_IPP
