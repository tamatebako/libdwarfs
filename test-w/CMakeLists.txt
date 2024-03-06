# Copyright (c) 2024 [Ribose Inc](https://www.ribose.com).
# All rights reserved.
# This file is a part of tebako
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required (VERSION 3.24)

# For project( ... VERSION ... )
cmake_policy(SET CMP0048 NEW)
# For CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)
# DOWNLOAD_EXTRACT_TIMESTAMP option default = true
cmake_policy(SET CMP0135 NEW)

# Various locations for external projects
set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps CACHE STRING "Dependencies' folder'")
set(DEPS_INCLUDE_DIR ${DEPS}/include)
set(DEPS_LIB_DIR ${DEPS}/lib)
set(DEPS_BIN_DIR ${DEPS}/bin)
set(DEPS_SBIN_DIR ${DEPS}/sbin)

if(NOT VCPKG_INSTALLED_DIR)
  set(VCPKG_INSTALLED_DIR "${DEPS}/vcpkg-installed")
endif(NOT VCPKG_INSTALLED_DIR)

if(NOT VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "x64-windows-static")
endif(NOT VCPKG_TARGET_TRIPLET)

project(libdwarfs-test-w VERSION "0.0.1")

include(ExternalProject)
include(GNUInstallDirs)
include(CheckSymbolExists)
include(CheckCXXSourceCompiles)

set(GNU_BASH "bash")

include(${CMAKE_SOURCE_DIR}/../tools/cmake-scripts/def-external-project.cmake)

include_directories(${DEPS_INCLUDE_DIR} ${DEPS_INCLUDE_DIR}/incbin ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
link_directories(${DEPS_LIB_DIR} ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib)


set(MKDWARFS ${DEPS_BIN_DIR}/mkdwarfs.exe)
set(INCBIN ${DEPS_BIN_DIR}/incbin.exe)

# DATA_BIN_DIR folder is used to create packaged filesystem
set(DATA_BIN_DIR  ${CMAKE_CURRENT_BINARY_DIR})
# DATA_BIN_FILE is packaged filesystem itself
set(DATA_BIN_FILE ${DATA_BIN_DIR}/fs.bin)


message(STATUS "Using mkdwarfs at ${MKDWARFS}")
message(STATUS "Using incbin at ${INCBIN}")
message(STATUS "Using packaged filesystem at ${DATA_BIN_FILE}")


def_ext_prj_t(LIBDWARFS_WR "0.5.0.rc7" "085eb1aeed100c5662ff9afff386c2f0cbe2065ddefe5adcd45a755b547532b0")

string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1;\\2;\\3" LIBDWARFS_WR_VER_COMPONENTS ${LIBDWARFS_WR_VER})

list(GET LIBDWARFS_WR_VER_COMPONENTS 0 LIBDWARFS_WR_VER_MAJOR)
list(GET LIBDWARFS_WR_VER_COMPONENTS 1 LIBDWARFS_WR_VER_MINOR)
list(GET LIBDWARFS_WR_VER_COMPONENTS 2 LIBDWARFS_WR_VER_PATCH)
set (LIBDWARFS_WR_VER_M ${LIBDWARFS_WR_VER_MAJOR}.${LIBDWARFS_WR_VER_MINOR}.${LIBDWARFS_WR_VER_PATCH})

ExternalProject_Add(${LIBDWARFS_WR_PRJ}
  PREFIX ${DEPS}
  URL https://github.com/tamatebako/libdwarfs/releases/download/v${LIBDWARFS_WR_VER}/libdwarfs-wr-${LIBDWARFS_WR_VER_M}-win64.7z
  URL_HASH SHA256=${LIBDWARFS_WR_HASH}
  DOWNLOAD_NO_PROGRESS true
  SOURCE_DIR ${LIBDWARFS_WR_SOURCE_DIR}
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPS}
          COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBDWARFS_WR_SOURCE_DIR} ${DEPS}
  TEST_COMMAND ""
)


# log level
if (NOT TESTS_LOG_LEVEL)
  set(TESTS_LOG_LEVEL "warn")
  message(STATUS "Logging level not specified. Defaulted to warn")
endif(NOT TESTS_LOG_LEVEL)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Build type was not specified. Defaulted to release")
  set(CMAKE_BUILD_TYPE Release)
endif()

set(Boost_USE_STATIC_LIBS   ON)
set(Boost_LINK_STATIC       ON)

# If Boost was built using the boost-cmake project or from Boost 1.70.0 on it provides
# a package configuration file for use with find_package's config mode.
# This configuration  file enforces -lz -lbz2 -llzma and it is an issue for us on Alpine/clang
# See https://cmake.org/cmake/help/latest/module/FindBoost.html#boost-cmake for details
set(Boost_NO_BOOST_CMAKE    ON)

set(GFLAGS_SHARED          OFF)
set(CMAKE_CXX_STANDARD      20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS   OFF)

set(CXX_STD "gnu++20" CACHE STRING "The C++ standard argument to pass to the compiler.")
set(MSVC_LANGUAGE_VERSION "c++20" CACHE STRING "The C++ standard argument to pass to the compiler.")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always -static-libgcc)

  # For gcc, -O3 is *much* worse than -O2
  # Update: This is still true for gcc-12
  set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2 -g")
  set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2 -g")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fcolor-diagnostics)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0.0")
    # for some reason, clang-10 binaries crash upon throwing exceptions
    # [maxirmx] This is true for clang-12 Release build, but no crash for RelWithDebugInfo
    add_compile_options(-fno-omit-frame-pointer)
  endif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0.0")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_compile_options(/Zc:__cplusplus /utf-8 /wd4267 /wd4244 /wd5219)
    # Apply /MT or /MTd  (multithread, static version of the run-time library)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()


if(DEFINED ENV{TEMP})
  file(TO_CMAKE_PATH "$ENV{TEMP}" TESTS_TMP_DIR)
elseif(DEFINED ENV{TMP})
  file(TO_CMAKE_PATH "$ENV{TMP}" TESTS_TMP_DIR)
else()
  message(FATAL_ERROR "Environment variable TEMP or TMP is not defined")
endif()

file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" TESTS_USR_DIR)
find_program(SHELL_PATH cmd)


# ...................................................................
# System libraries
# For Windwos build these libraries are installed with vcpkg
#

find_package(Boost 1.65 REQUIRED COMPONENTS chrono date_time iostreams filesystem program_options thread system)

find_library(_LIBGLOG "glog" REQUIRED)
if(CMAKE_BUILD_TYPE STREQUAL Release)
  find_library(_LIBGFLAGS "gflags_static" REQUIRED)
else(CMAKE_BUILD_TYPE STREQUAL Release)
  find_library(_LIBGFLAGS "gflags_static_debug" REQUIRED)
endif(CMAKE_BUILD_TYPE STREQUAL Release)
find_library(_LIBDC "double-conversion" REQUIRED)
find_library(_LIBBROTLICOMMON "brotlicommon")
find_library(_LIBBROTLIENC "brotlienc")
find_library(_LIBBROTLIDEC "brotlidec")
find_library(_LIBLZ4 "lz4")
find_library(_LIBEVENT event REQUIRED)
find_library(_LIBLZMA lzma REQUIRED)
find_library(_LIBZ zlib REQUIRED)
find_library(_LIBSSL libssl REQUIRED)
find_library(_LIBCRYPTO libcrypto REQUIRED)

find_library(_LIBZSTD zstd REQUIRED)
find_library(_LIBARCHIVE archive REQUIRED)
find_library(_LIBXXHASH xxhash REQUIRED)
if(CMAKE_BUILD_TYPE STREQUAL Release)
  find_library(_LIBFMT fmt REQUIRED)
else(CMAKE_BUILD_TYPE STREQUAL Release)
  find_library(_LIBFMT fmtd REQUIRED)
endif(CMAKE_BUILD_TYPE STREQUAL Release)
set(_LIBEH "")

find_library(_LIBDWARFS_WR dwarfs-wr REQUIRED HINTS ${DEPS_LIB_DIR})
find_library(_LIBDWARFS dwarfs REQUIRED HINTS ${DEPS_LIB_DIR})
find_library(_LIBFOLLY folly REQUIRED HINTS ${DEPS_LIB_DIR})
find_library(_LIBFSST fsst REQUIRED HINTS ${DEPS_LIB_DIR})
find_library(_LIBMETADATA_THRIFT metadata_thrift REQUIRED HINTS ${DEPS_LIB_DIR})
find_library(_LIBTHRIFT_LIGHT thrift_light REQUIRED HINTS ${DEPS_LIB_DIR})


# Packaged filesystem

add_custom_command(
  OUTPUT ${DATA_BIN_FILE}
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/tebako-fs.c
  COMMAND ${MKDWARFS} --force --progress simple -o ${DATA_BIN_FILE} -i ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_filesystem
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}/directory-1/*empty.*
  DEPENDS ${DWARFS_WR_PRJ}
)

configure_file(
${CMAKE_CURRENT_SOURCE_DIR}/resources/tebako-fs.c.in
${CMAKE_CURRENT_SOURCE_DIR}/tebako-fs.c
@ONLY
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tebako-fs0.c
  COMMAND ${DEPS}/bin/incbin.exe ${CMAKE_CURRENT_SOURCE_DIR}/tebako-fs.c -o ${CMAKE_CURRENT_SOURCE_DIR}/tebako-fs0.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tebako-fs.c ${DATA_BIN_FILE} ${DWARFS_WR_PRJ}
)

add_executable(wr-test-w
  "test-w.c"
  "tebako-fs.h"
  "tebako-fs0.c"
)

add_dependencies(wr-test-w ${LIBDWARFS_WR_PRJ})

message(STATUS ${Boost_LIBRARIES})

target_link_libraries(wr-test-w PRIVATE
      ${_LIBDWARFS_WR} ${_LIBDWARFS} ${_LIBFOLLY} ${_LIBFSST} ${_LIBMETADATA_THRIFT} ${_LIBTHRIFT_LIGHT}
      ${_LIBGLOG} ${_LIBGFLAGS} ${_LIBDC} ${_LIBBROTLICOMMON} ${_LIBBROTLIENC} ${_LIBBROTLIDEC}
      ucrt crypt32 shlwapi wsock32 ws2_32)
