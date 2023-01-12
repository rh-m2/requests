cmake_minimum_required(VERSION 3.20)
project(requests)

set(CMAKE_CXX_STANDARD 20)

option(BOOST_REQUESTS_BUILD_INLINE "Configure as if part of the boost source tree" OFF)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

if(BOOST_REQUESTS_BUILD_INLINE)

    add_library(boost_requests src/src.cpp)
    target_include_directories(boost_requests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(boost_requests PUBLIC 
        Boost::asem Boost::beast Boost::container Boost::url Boost::system Boost::thread 
        OpenSSL::SSL OpenSSL::Crypto 
        Threads::Threads)
    target_compile_options(boost_requests PUBLIC "-DBOOST_REQUESTS_SEPARATE_COMPILATION=1")
    add_library(Boost::requests ALIAS boost_requests)

else()

    find_package(Boost REQUIRED system container)


    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)

    include_directories(include libs/url/include)

    add_subdirectory(test)
    add_subdirectory(example)

endif()
