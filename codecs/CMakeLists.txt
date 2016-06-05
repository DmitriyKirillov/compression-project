cmake_minimum_required(VERSION 3.0)

project(codecs)

enable_testing()
include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/common.cmake)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Ofast -Wall -Wextra -Werror -fsanitize=address -fsanitize=undefined -fsanitize=leak")
endif()

#set(Boost_USE_STATIC_LIBS ON)
#set(Boost_USE_MULTITHREADED OFF)
#set(Boost_NO_SYSTEM_PATHS ON)
#
#set(Boost_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/external/boost)
#set(Boost_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/external/boost/lib)
#
#
#find_package(Boost COMPONENTS program_options REUIRED)
#
#INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
#LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/boost)


add_subdirectory(external)
add_subdirectory(library)
add_subdirectory(tools)
