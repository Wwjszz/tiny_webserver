cmake_minimum_required(VERSION 3.26.5)

project(tiny_webserver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/usr/lib64/mysql -lmysqlclient -lpthread")
message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

add_executable(webserver main.cpp)
file(GLOB sources src/*.cc)
target_sources(webserver PUBLIC ${sources})

