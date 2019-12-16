cmake_minimum_required(VERSION 3.15)
project(pardus)

set(CMAKE_CXX_STANDARD 14)

include_directories(public_html)
include_directories(src)

add_executable(pardus
        public_html/index.html
        src/pd_http.cpp
        src/pd_http.h
        src/pd_http_server.cpp
        src/pd_http_server.h
        src/pd_net.cpp
        src/pd_net.h
        src/pd_util.cpp
        src/pd_util.h
        LICENSE
        README.md)
