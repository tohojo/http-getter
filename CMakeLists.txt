cmake_minimum_required(VERSION 2.8)
project(http-getter)

set(CPACK_GENERATOR "DEB")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
set(CPACK_STRIP_FILES 1)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_CONTACT "Toke Høiland-Jørgensen <toke@toke.dk>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Parallel HTTP URL getter built on libcurl.")

IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
ELSE()
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.7), libcurl3")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")
include(CPack)

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
  src/getter.c
  src/worker.c)
set(http-getter_HEADERS
  include/options.h
  include/getter.h)

include_directories(include/)

add_executable(http-getter
  ${http-getter_HEADERS}
  ${http-getter_SOURCES}
  ${CURL_INCLUDE_DIRS})
target_link_libraries(http-getter m curl ${CURL_LIBRARIES})

install(TARGETS http-getter DESTINATION bin)
