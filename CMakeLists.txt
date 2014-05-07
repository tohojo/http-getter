cmake_minimum_required(VERSION 2.8)
project(http-getter)
find_package(PkgConfig)

FIND_PACKAGE(CURL)
IF(CURL_FOUND)
  MESSAGE(STATUS "Curl libraries found at: ${CURL_LIBRARIES}")
  MESSAGE(STATUS "Curl includes found at: ${CURL_INCLUDE_DIRS}")
else()
  MESSAGE(SEND_ERROR "Could not find cURL on your system")
ENDIF(CURL_FOUND)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long")


set(http-getter_SOURCES
  src/main.c
  src/options.c
  src/getter.c)
set(http-getter_HEADERS
  include/options.h
  include/getter.h)

include_directories(include/)

add_executable(http-getter
  ${http-getter_HEADERS}
  ${http-getter_SOURCES}
  ${CURL_INCLUDE_DIRS})
target_link_libraries(http-getter m curl ${CURL_LIBRARIES})
