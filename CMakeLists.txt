# Copyright (c) 2021-2024 [Ribose Inc](https://www.ribose.com).
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
# FindBoost moudule
# Pls refer to the comment re 'set(Boost_NO_BOOST_CMAKE ON)' below
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.30.0")
  cmake_policy(SET CMP0167 OLD)
endif(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.30.0")

include(tools/cmake-scripts/version.cmake)
determine_version("${CMAKE_CURRENT_SOURCE_DIR}" LIBDWARFS_WR)

# Various locations for external projects
set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps CACHE STRING "Dependencies' folder'")
set(DEPS_INCLUDE_DIR ${DEPS}/include)
set(DEPS_LIB_DIR ${DEPS}/lib)
set(DEPS_BIN_DIR ${DEPS}/bin)
set(DEPS_SBIN_DIR ${DEPS}/sbin)

if(MSVC)
  if(NOT VCPKG_INSTALLED_DIR)
    set(VCPKG_INSTALLED_DIR "${DEPS}/vcpkg-installed")
  endif(NOT VCPKG_INSTALLED_DIR)
  if(NOT VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static")
  endif(NOT VCPKG_TARGET_TRIPLET)
endif(MSVC)

project(libdwarfs-wr VERSION ${LIBDWARFS_WR_VERSION})

option(PREFER_SYSTEM_GTEST "Use system Google test" OFF)
option(WITH_TESTS "Find Google test, install INCBIN, build test applications" ON)
# Visual Studio/Windows Subsystem for Linux (WSL) environment requires
# source tree to be placed to native Windows partiotion
# Native Windows partition does not support links
# So, two options are provided below:
# -- disable links tests: set WITH_LINK_TEST to OFF
# -- place test data to WSL native partition

option(WITH_LINK_TESTS "Include tests for hard and symbolic links" ON)
option(USE_TEMP_FS "Place test data to WSL native partition" OFF)

include(ExternalProject)
include(GNUInstallDirs)
include(CheckSymbolExists)
include(CheckCXXSourceCompiles)

# This is the default that will be overwritten for MacOS
set(GNU_BASH "bash")

include(${CMAKE_SOURCE_DIR}/tools/cmake-scripts/macos-environment.cmake)
include(${CMAKE_SOURCE_DIR}/tools/cmake-scripts/def-external-project.cmake)

include_directories(${DEPS_INCLUDE_DIR})
link_directories(${DEPS_LIB_DIR})
if(MSVC)
  include_directories(${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
  link_directories(${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib)
endif(MSVC)

# log level
if (NOT TESTS_LOG_LEVEL)
  set(TESTS_LOG_LEVEL "warn")
  message(STATUS "Logging level not specified. Defaulted to warn")
endif(NOT TESTS_LOG_LEVEL)

# coverage
if (WITH_COVERAGE)
  if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Coverage is only supported with the GNU compiler")
  endif()
  add_compile_options(--coverage -O0)
  add_link_options(--coverage)
endif(WITH_COVERAGE)

# sanitizers
if (WITH_ASAN)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Sanitizers are only supported with the clang compiler")
  endif()
  add_compile_options(-DWITH_ASAN -fsanitize=address,undefined -fno-omit-frame-pointer -fno-common -O1)
  link_libraries(-fsanitize=address,undefined)
endif(WITH_ASAN)

if (WITH_COVERAGE OR WITH_ASAN)
  message(STATUS "Forcing WITH_TEST=ON for code coverage or sanitizers")
  set(WITH_TESTS ON CACHE BOOL "Forced for code coverage or sanitizers" FORCE)
  message(STATUS "Forcing build type to Debug for code coverage or sanitizers")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Forced for code coverage or sanitizers" FORCE)
endif(WITH_COVERAGE OR WITH_ASAN)

if(MKDWARFS)
  set(TEBAKO_BUILD_SCOPE LIB)
  message(STATUS "Using upstream mkdwarfs at ${MKDWARFS}")
else(MKDWARFS)
  set(TEBAKO_BUILD_SCOPE MKD)
  set(MKDWARFS ${DEPS_BIN_DIR}/mkdwarfs)
  message(STATUS "Building mkdwarfs at ${MKDWARFS}")
endif(MKDWARFS)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Build type was not specified. Defaulted to release")
  set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# Reasonable defaults
set(USE_JEMALLOC ON)
set(WITH_JEMALLOC_BUILD OFF)
set(JEMALLOC_LG_VADDR "")
set(JEMALLOC_LG_VADDR_MSG "")
set(JEMALLOC_LG_PAGE "")
set(JEMALLOC_LG_PAGE_MSG "")
set(WITH_GLOG_BUILD OFF)
set(WITH_GFLAGS_BUILD OFF)
set(WITH_DC_BUILD OFF)
set(WITH_LZ4_BUILD OFF)
set(WITH_FMT_BUILD OFF)
set(WITH_ZSTD_BUILD OFF)

set(IS_MUSL OFF)
set(IS_WINDOWS OFF)
set(IS_MSYS OFF)
set(IS_DARWIN OFF)

if(MSVC)
  set(IS_WINDOWS ON)
  set(OSTYPE_TXT "windows")
  set(OSTYPE_RES  0)
  set(USE_JEMALLOC OFF)
  set(VCPKG_PARAMS -GNinja -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET} -DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR})
  set(WITH_LINK_TESTS OFF)
  set(LIBEMPTY_NAME "empty.dll")
  set(MKDWARFS "${MKDWARFS}.exe")
  add_compile_options(/Zc:__cplusplus /utf-8 /wd4267 /wd4244 /wd5219)
  # Apply /MT or /MTd  (multithread, static version of the run-time library)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(MINGW)
  set(IS_MSYS ON)
  set(OSTYPE_TXT "msys")
  set(OSTYPE_RES  0)
  set(USE_JEMALLOC OFF)
  set(PREFER_SYSTEM_GTEST ON)
  set(WITH_LINK_TESTS OFF)
  set(LIBEMPTY_NAME "libempty.dll")
  set(MKDWARFS "${MKDWARFS}.exe")
  set(WITH_FMT_BUILD ON)
else()
  set(LIBEMPTY_NAME "libempty.so")
  set(WITH_ZSTD_BUILD ON)
  execute_process(
    COMMAND "${GNU_BASH}"
            "-c"
            "echo \$OSTYPE"
    RESULT_VARIABLE OSTYPE_RES
    OUTPUT_VARIABLE OSTYPE_TXT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  if("${OSTYPE_TXT}" MATCHES "^linux-musl.*")
    set(IS_MUSL ON)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Alpine 3.17++ lz4 & brotli libraries use lto and I did not find tool's configuration that works
    # (binutils-gold triggers issues with libstdc++)
      set(WITH_LZ4_BUILD ON)
      set(WITH_BROTLI_BUILD ON)
    endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  elseif("${OSTYPE_TXT}" MATCHES "^darwin.*")
    set(IS_DARWIN ON)
    message(STATUS "Building jemalloc library locally")
    set(WITH_JEMALLOC_BUILD ON)
    if(DEFINED ENV{LG_VADDR})
      set(JEMALLOC_LG_VADDR  --with-lg-vaddr=$ENV{LG_VADDR})
      set(JEMALLOC_LG_VADDR_MSG ", lg_vaddr = $ENV{LG_VADDR}")
      message(STATUS "Using LG_VADDR environment variable: ${JEMALLOC_LG_VADDR}")
    endif(DEFINED ENV{LG_VADDR})
    if(DEFINED ENV{LG_PAGE})
      set(JEMALLOC_LG_PAGE  --with-lg-page=$ENV{LG_PAGE})
      set(JEMALLOC_LG_PAGE_MSG ", lg_page = $ENV{LG_PAGE}")
      message(STATUS "Using LG_PAGE environment variable: ${JEMALLOC_LG_PAGE}")
  endif(DEFINED ENV{LG_PAGE})

  endif()
endif(MSVC)

# RB_W32
# -----------------------------------------------------------------------------
#  - windows (enforced)         will have IS_WINDOWS flag set
#    actually it is defined by compiler only (i.e.: MSVC) and my need revision
#    RB_W32 setting calls for Ruby style simulation of IO
#   (enable it by default when running on Windows)
#
#    RB_W32 means use of Ruby Linux-like Dir IO functions
#    Possible other methods may be MinGW Dir IO (== posix), folly portability (== posix)
#    These two most likely would work but are not tested
if(NOT IS_WINDOWS AND NOT IS_MSYS)
  set(RB_W32 OFF)
else(NOT IS_WINDOWS AND NOT IS_MSYS)
  set(RB_W32 ON)
endif(NOT IS_WINDOWS AND NOT IS_MSYS)
# -----------------------------------------------------------------------------
#  - "darwin.*"                 will have IS_DARWIN flag set
# -----------------------------------------------------------------------------
#  - "linux-musl.*"             will have IS_MUSL flag set
# -----------------------------------------------------------------------------
#  - ... everything else ...    will be be treated as linux-gnu but in reality
#                               only Ubuntu is supported/tested

if(OSTYPE_RES EQUAL 0)
  message(STATUS "OSTYPE: '${OSTYPE_TXT}'")
else(OSTYPE_RES EQUAL 0)
  message(FATAL_ERROR "Failed to detect OSTYPE: ${OSTYPE_TXT}")
endif(OSTYPE_RES EQUAL 0)

message("Configuraion summary:")
message(STATUS "Tebako build scope: ${TEBAKO_BUILD_SCOPE}")
if (IS_WINDOWS)
  message(STATUS "Building for Ruby Win32: ${RB_W32}")
endif(IS_WINDOWS)
message(STATUS "Use jemalloc: ${USE_JEMALLOC}${JEMALLOC_LG_VADDR_MSG}${JEMALLOC_LG_PAGE_MSG}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  with ASAN: ${WITH_ASAN}")
message(STATUS "  with coverage: ${WITH_COVERAGE}")
message(STATUS "  with tests: ${WITH_TESTS}")
if(WITH_TESTS)
  message(STATUS "    using mkdwarfs at: ${MKDWARFS}")
  message(STATUS "    test applications logging: ${TESTS_LOG_LEVEL}")
  message(STATUS "    with link tests: ${WITH_LINK_TESTS}")
  message(STATUS "    using temporary test directory: ${USE_TEMP_FS}")
endif(WITH_TESTS)
if(VCPKG_PARAMS)
  message(STATUS "Using vcpkg with: ${VCPKG_PARAMS}")
endif(VCPKG_PARAMS)

set(Boost_USE_STATIC_LIBS   ON)
set(Boost_LINK_STATIC       ON)

# If Boost was built using the boost-cmake project or from Boost 1.70.0 on it provides
# a package configuration file for use with find_package's config mode.
# This configuration  file enforces -lz -lbz2 -llzma and it is an issue for us on Alpine/clang
# See https://cmake.org/cmake/help/latest/module/FindBoost.html#boost-cmake for details
set(Boost_NO_BOOST_CMAKE    ON)

set(USE_STATIC_DEPS_ON_UNIX ON)
set(GFLAGS_SHARED          OFF)
set(CMAKE_CXX_STANDARD      20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS   OFF)

# This is due to a bug in folly's Portability.h
# Adopted 'as is' from Dwarfs CMake script
add_compile_definitions(FOLLY_CFG_NO_COROUTINES)
add_compile_definitions(GLOG_NO_ABBREVIATED_SEVERITIES NOMINMAX NOGDI)
set(CXX_STD "gnu++20" CACHE STRING "The C++ standard argument to pass to the compiler.")
set(MSVC_LANGUAGE_VERSION "c++20" CACHE STRING "The C++ standard argument to pass to the compiler.")


if(CMAKE_BUILD_TYPE STREQUAL Debug)
# Mask folly symbolizer issue where it includes <dwarf.h> instead of <libdwarf/dwarf.h>
  file(WRITE ${DEPS_INCLUDE_DIR}/dwarf.h "#include <libdwarf/dwarf.h>")
endif(CMAKE_BUILD_TYPE STREQUAL Debug)



# ...................................................................
# External projects

def_ext_prj_g(DWARFS "tebako-v0.8.2")
def_ext_prj_g(INCBIN "348e36b")

if(NOT IS_WINDOWS)
  find_library(_LIBGLOG "libglog.a")
  if(${_LIBGLOG} STREQUAL "_LIBGLOG-NOTFOUND")
    message(STATUS "glog static library was not found, building locally")
    set(WITH_GLOG_BUILD ON)
    if(IS_DARWIN)
      message(STATUS "building gflags locally as well because of MacOS specifics")
      set(WITH_GFLAGS_BUILD ON)
    endif(IS_DARWIN)
  endif()

  if (NOT WITH_GFLAGS_BUILD)
    find_library(_LIBGFLAGS "libgflags.a")
    if(${_LIBGFLAGS} STREQUAL "_LIBGFLAGS-NOTFOUND")
      message(STATUS "gflags static library was not found, building locally")
      set(WITH_GFLAGS_BUILD ON)
    endif()
  endif()

  find_library(_LIBDC "libdouble-conversion.a")
  if(${_LIBDC} STREQUAL "_LIBDC-NOTFOUND")
    message(STATUS "double-conversion static library was not found, building locally")
    set(WITH_DC_BUILD ON)
  endif()

  if(NOT WITH_BROTLI_BUILD AND NOT IS_MSYS)
    find_library(_LIBBROTLICOMMON "libbrotlicommon.a")
    find_library(_LIBBROTLIENC "libbrotlienc.a")
    find_library(_LIBBROTLIDEC "libbrotlidec.a")
    if(${_LIBBROTLICOMMON} STREQUAL "_LIBBROTLICOMMON-NOTFOUND" OR
      ${_LIBBROTLIENC} STREQUAL "_LIBBROTLIENC-NOTFOUND" OR
      ${_LIBBROTLIDEC} STREQUAL "_LIBBROTLIDEC-NOTFOUND")
      message(STATUS "brotli static library was not found, building locally")
      set(WITH_BROTLI_BUILD ON)
    endif()
  endif(NOT WITH_BROTLI_BUILD AND NOT IS_MSYS)

  if(NOT WITH_LZ4_BUILD)
    find_library(_LIBLZ4 "liblz4.a")
    if(${_LIBLZ4} STREQUAL "_LIBLZ4-NOTFOUND")
      message(STATUS "lz4 static library was not found, building locally")
      set(WITH_LZ4_BUILD ON)
    endif()
  endif(NOT WITH_LZ4_BUILD)
  if(WITH_JEMALLOC_BUILD)
    def_ext_prj_t(JEMALLOC "5.2.1" "34330e5ce276099e2e8950d9335db5a875689a4c6a56751ef3b1d8c537f887f6")
  elseif(WITH_JEMALLOC_BUILD)
    find_library(_LIBJEMALLOC "libjemalloc.a" REQUIRED)
  endif(WITH_JEMALLOC_BUILD)

else(NOT IS_WINDOWS)
  set(WITH_DLFCN_WIN32_BUILD ON)
endif(NOT IS_WINDOWS)

message("Collecting packages:")
message(STATUS "dwarfs      -  @${DWARFS_TAG}   at  ${DWARFS_SOURCE_DIR}")
message(STATUS "incbin      -  @${INCBIN_TAG}  at  ${INCBIN_SOURCE_DIR}")
if(WITH_ZSTD_BUILD)
  def_ext_prj_g(ZSTD "v1.5.5")
  message(STATUS "zstd        -  @${ZSTD_TAG}  at  ${ZSTD_SOURCE_DIR}")
endif(WITH_ZSTD_BUILD)
if(WITH_FMT_BUILD)
  def_ext_prj_g(FMT "10.1.1")
  message(STATUS "fmt         -  @${FMT_TAG}  at  ${FMT_SOURCE_DIR}")
endif(WITH_FMT_BUILD)
if(WITH_GLOG_BUILD)
  def_ext_prj_g(GLOG "v0.6.0")
  message(STATUS "glog        -  @${GLOG_TAG}  at  ${GLOG_SOURCE_DIR}")
endif(WITH_GLOG_BUILD)
if(WITH_GFLAGS_BUILD)
  def_ext_prj_g(GFLAGS "v2.2.2")
  message(STATUS "gflags      -  @${GFLAGS_TAG}  at  ${GFLAGS_SOURCE_DIR}")
endif(WITH_GFLAGS_BUILD)
if(WITH_DC_BUILD)
  def_ext_prj_g(DC "v3.2.0")
  message(STATUS "double-conversion  -  @${DC_TAG}  at  ${DC_SOURCE_DIR}")
endif(WITH_DC_BUILD)
if(WITH_BROTLI_BUILD)
  def_ext_prj_g(BROTLI "v1.1.0")
  message(STATUS "brotli      -  @${BROTLI_TAG}  at  ${BROTLI_SOURCE_DIR}")
endif(WITH_BROTLI_BUILD)
if(WITH_LZ4_BUILD)
  def_ext_prj_g(LZ4 "v1.9.4")
  message(STATUS "lz4         -  @${LZ4_TAG}  at  ${LZ4_SOURCE_DIR}")
endif(WITH_LZ4_BUILD)
if(WITH_JEMALLOC_BUILD)
  message(STATUS "jemalloc    -  v${JEMALLOC_VER}  at  ${JEMALLOC_SOURCE_DIR}")
endif(WITH_JEMALLOC_BUILD)
if(WITH_DLFCN_WIN32_BUILD)
  def_ext_prj_g(DLFCN_WIN32 "v1.4.1")
  message(STATUS "dlfcn-win32 -  @${DLFCN_WIN32_TAG}  at  ${DLFCN_WIN32_SOURCE_DIR}")
endif(WITH_DLFCN_WIN32_BUILD)

# ...................................................................
# incbin

if(WITH_TESTS)
# ...................................................................
# Filesystem locations

# DATA_TEST_DIR folder contains raw data for filesystem tests
  if(USE_TEMP_FS)
    string(RANDOM LENGTH 10 TMPNM)
    set(DATA_TEST_DIR "/tmp/${TMPNM}")
    set(DATA_TEST_DIR_MSG "at '${DATA_TEST_DIR}'")
  else(USE_TEMP_FS)
    set(DATA_TEST_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filesystem)
    set(DATA_TEST_DIR_MSG "")
  endif(USE_TEMP_FS)

  if (WITH_LINK_TESTS)
    set(WLT "with link tests")
    add_compile_definitions(WITH_LINK_TESTS)
  else(WITH_LINK_TESTS)
    set(WLT "without link tests")
  endif(WITH_LINK_TESTS)

  message(STATUS "Using test data set ${WLT} from '${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filesystem' ${DATA_TEST_DIR_MSG}")

# DATA_BIN_DIR folder is used to create packaged filesystem
  set(DATA_BIN_DIR  ${CMAKE_CURRENT_BINARY_DIR})
# DATA_BIN_FILE is packaged filesystem itself
  set(DATA_BIN_FILE ${DATA_BIN_DIR}/fs.bin)

endif(WITH_TESTS)

# ...................................................................
# DwarFS

# ...................................................................
# The libraries that are build by DwarFS project
# libdwarfs libdwarfs_compression
# libfolly libfsst libxxhash libmetadata_thrift libthrift_light
# libfmt libarchive
# These forward-declarations and BUILD_BYPRODICTS are required to support 'Ninja'
# Otherwise add_dependencies would be enough for 'Unix makefiles' generator

set(__LIBDWARFS "${DWARFS_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}dwarfs${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(__LIBDWARFS_COMPRESSION "${DWARFS_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}dwarfs_compression${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(__LIBFOLLY "${DWARFS_BINARY_DIR}/folly/${CMAKE_STATIC_LIBRARY_PREFIX}folly${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(__LIBFSST "${DWARFS_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}fsst${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(__LIBT_METADATA "${DWARFS_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}metadata_thrift${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(__LIBT_LIGHT "${DWARFS_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}thrift_light${CMAKE_STATIC_LIBRARY_SUFFIX}")

if(NOT IS_WINDOWS)
  if(IS_MSYS)
    set(__LIBARCHIVE "${DEPS}/lib/libarchive_static.a")
  else(IS_MSYS)
    set(__LIBARCHIVE "${DEPS}/lib/libarchive.a")
  endif(IS_MSYS)
endif(NOT IS_WINDOWS)

if(NOT IS_WINDOWS AND NOT IS_MSYS)
  # Use system zstd and xxhash for MSys
  set(__LIBXXHASH "${DWARFS_BINARY_DIR}/libxxhash.a")

  # Build fmt for MSys
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(__LIBFMT "${DWARFS_BINARY_DIR}/_deps/fmt-build/libfmtd.a")
  else()
    set(__LIBFMT "${DWARFS_BINARY_DIR}/_deps/fmt-build/libfmt.a")
  endif()
endif(NOT IS_WINDOWS AND NOT IS_MSYS)

ExternalProject_Add(${DWARFS_PRJ}
  PREFIX ${DEPS}
  GIT_REPOSITORY https://github.com/tamatebako/dwarfs.git
  GIT_TAG ${DWARFS_TAG}
  UPDATE_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
             ${VCPKG_PARAMS}
             -DWITH_TESTS=OFF
             -DTEBAKO_BUILD_SCOPE=${TEBAKO_BUILD_SCOPE}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DDEPS=${DEPS}
             -DCMAKE_PREFIX_PATH=${DEPS}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DUSE_JEMALLOC=${USE_JEMALLOC}
             -DFOLLY_NO_EXCEPTION_TRACER=ON
             -DDISABLE_CCACHE=ON
             -DWITH_MAN_PAGES=OFF
             -DNIXPKGS_DWARFS_VERSION_OVERRIDE=tebako-${LIBDWARFS_WR_VERSION}
  SOURCE_DIR ${DWARFS_SOURCE_DIR}
  BINARY_DIR ${DWARFS_BINARY_DIR}
  BUILD_BYPRODUCTS ${__LIBDWARFS}       ${__LIBDWARFS_COMPRESSION} ${__LIBFSST}   ${__LIBFOLLY}
                   ${__LIBT_METADATA}   ${__LIBT_LIGHT}            ${__LIBXXHASH} ${__LIBZSTD}
                   ${__LIBFMT}          ${__LIBARCHIVE}
)

# ...................................................................
# incbin

set(INCBIN_BUILD_COMMAND ${CMAKE_COMMAND} -E true)
if(IS_WINDOWS)
  set(INCBIN_BUILD_COMMAND ${CMAKE_C_COMPILER} ${INCBIN_SOURCE_DIR}/incbin.c -nologo -link -out:${DEPS_BIN_DIR}/incbin.exe)
endif(IS_WINDOWS)

ExternalProject_Add(${INCBIN_PRJ}
  PREFIX ${DEPS}
  GIT_REPOSITORY https://github.com/graphitemaster/incbin.git
  GIT_TAG ${INCBIN_TAG}
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPS}/bin
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPS}/include
  BUILD_COMMAND ${INCBIN_BUILD_COMMAND}
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${INCBIN_SOURCE_DIR}/incbin.h ${DEPS}/include/incbin.h
)

# ...................................................................
# zstd

if(WITH_ZSTD_BUILD)
  set(__LIBZSTD "${DEPS}/lib/libzstd.a")
  ExternalProject_Add(${ZSTD_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/facebook/zstd.git"
    GIT_TAG ${ZSTD_TAG}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
               -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DBUILD_SHARED_LIBS:BOOL=OFF
               -DZSTD_BUILD_PROGRAMS:BOOL=OFF
               -DBUILD_TESTING:BOOL=OFF
               -DCMAKE_CXX_STANDARD=17
               -DCMAKE_CXX_STANDARD_REQUIRED=ON
               -DCMAKE_CXX_EXTENSIONS=OFF
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -S${ZSTD_SOURCE_DIR}/build/cmake
               -B${ZSTD_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBZSTD}
  )
  add_dependencies(${DWARFS_PRJ} ${ZSTD_PRJ})
endif(WITH_ZSTD_BUILD)

# ...................................................................
# fmt

if(WITH_FMT_BUILD)
  set(__LIBFMT "${DEPS}/lib/libfmt.a")
  ExternalProject_Add(${FMT_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
    GIT_TAG ${FMT_TAG}
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DBUILD_SHARED_LIBS:BOOL=OFF
               -DCMAKE_CXX_STANDARD=17
               -DCMAKE_CXX_STANDARD_REQUIRED=ON
               -DCMAKE_CXX_EXTENSIONS=OFF
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    SOURCE_DIR ${FMT_SOURCE_DIR}
    BINARY_DIR ${FMT_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBFMT}
  )
  add_dependencies(${DWARFS_PRJ} ${FMT_PRJ})
endif(WITH_FMT_BUILD)

# ...................................................................
# glog

if(WITH_GLOG_BUILD)
  set(__LIBGLOG "${DEPS}/lib/libglog.a")
  ExternalProject_Add(${GLOG_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/google/glog.git"
    GIT_TAG ${GLOG_TAG}
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DBUILD_SHARED_LIBS:BOOL=OFF
               -DBUILD_TESTING=OFF
               -DWITH_GTEST=OFF
               -DCMAKE_CXX_STANDARD=17
               -DCMAKE_CXX_STANDARD_REQUIRED=ON
               -DCMAKE_CXX_EXTENSIONS=OFF
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DWITH_UNWIND=OFF
    SOURCE_DIR ${GLOG_SOURCE_DIR}
    BINARY_DIR ${GLOG_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBGLOG}
  )
  add_dependencies(${DWARFS_PRJ} ${GLOG_PRJ})
endif(WITH_GLOG_BUILD)

# ...................................................................
# gflags

if(WITH_GFLAGS_BUILD)
  set(__LIBGFLAGS "${DEPS}/lib/libgflags.a")
  ExternalProject_Add(${GFLAGS_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/gflags/gflags.git"
    GIT_TAG ${GFLAGS_TAG}
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DGFLAGS_BUILD_STATIC_LIBS:BOOL=ON
               -DGFLAGS_BUILD_SHARED_LIBS:BOOL=OFF
               -DBUILD_gflags_nothreads_LIB:BOOL=OFF
               -DINSTALL_SHARED_LIBS:BOOL=OFF
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    SOURCE_DIR ${GFLAGS_SOURCE_DIR}
    BINARY_DIR ${GFLAGS_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBGFLAGS}
  )
  add_dependencies(${GLOG_PRJ} ${GFLAGS_PRJ})
  add_dependencies(${DWARFS_PRJ} ${GFLAGS_PRJ})
endif(WITH_GFLAGS_BUILD)

# ...................................................................
# double-conversion

if(WITH_DC_BUILD)
  set(__LIBDC "${DEPS}/lib/libdouble-conversion.a")
  ExternalProject_Add(${DC_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/google/double-conversion.git"
    GIT_TAG ${DC_TAG}
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    SOURCE_DIR ${DC_SOURCE_DIR}
    BINARY_DIR ${DC_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBDC}
  )
  add_dependencies(${DWARFS_PRJ} ${DC_PRJ})
endif(WITH_DC_BUILD)

# ...................................................................
# brotli

if(WITH_BROTLI_BUILD)
  set(__LIBBROTLICOMMON "${DEPS}/lib/libbrotlicommon.a")
  set(__LIBBROTLIENC "${DEPS}/lib/libbrotlienc.a")
  set(__LIBBROTLIDEC "${DEPS}/lib/libbrotlidec.a")
  ExternalProject_Add(${BROTLI_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/google/brotli.git"
    GIT_TAG ${BROTLI_TAG}
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DBUILD_SHARED_LIBS=OFF
               -DBROTLI_BUNDLED_MODE=OFF
    SOURCE_DIR ${BROTLI_SOURCE_DIR}
    BINARY_DIR ${BROTLI_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBBROTLICOMMON} ${__LIBBROTLIENC} ${__LIBBROTLIDEC}
  )
  add_dependencies(${DWARFS_PRJ} ${BROTLI_PRJ})
endif(WITH_BROTLI_BUILD)

# ...................................................................
# lz4

if(WITH_LZ4_BUILD)
  set(__LIBLZ4 "${DEPS}/lib/liblz4.a")
  ExternalProject_Add(${LZ4_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/lz4/lz4.git"
    GIT_TAG ${LZ4_TAG}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE true
    BUILD_COMMAND cd "${LZ4_SOURCE_DIR}" && make "prefix=${DEPS}"
    INSTALL_COMMAND cd "${LZ4_SOURCE_DIR}" && make "prefix=${DEPS}" install
    SOURCE_DIR ${LZ4_SOURCE_DIR}
    BUILD_BYPRODUCTS ${__LIBLZ4}
  )
  add_dependencies(${DWARFS_PRJ} ${LZ4_PRJ})
endif(WITH_LZ4_BUILD)

# ...................................................................
# jemalloc

if(WITH_JEMALLOC_BUILD)
  set(__LIBJEMALLOC "${DEPS}/lib/libjemalloc.a")

  ExternalProject_Add(${JEMALLOC_PRJ}
    PREFIX ${DEPS}
    URL https://github.com/jemalloc/jemalloc/releases/download/${JEMALLOC_VER}/jemalloc-${JEMALLOC_VER}.tar.bz2
    URL_HASH SHA256=${JEMALLOC_HASH}
    DOWNLOAD_NO_PROGRESS true
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ${JEMALLOC_SOURCE_DIR}/configure
                      --srcdir=${JEMALLOC_SOURCE_DIR}
                      --prefix=${DEPS}
                      --enable-static
                      --disable-shared
                      --datarootdir=${DEPS}/share.dummy
                      --disable-syscall
                      ${JEMALLOC_LG_VADDR} ${JEMALLOC_LG_PAGE}
    SOURCE_DIR ${JEMALLOC_SOURCE_DIR}
    BINARY_DIR ${JEMALLOC_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBJEMALLOC}
  )
endif(WITH_JEMALLOC_BUILD)

# ...................................................................
# dlfcn-win32
if(WITH_DLFCN_WIN32_BUILD)
  ExternalProject_Add(${DLFCN_WIN32_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/dlfcn-win32/dlfcn-win32"
    GIT_TAG ${DLFCN_WIN32_TAG}
    UPDATE_COMMAND ""
    SOURCE_DIR ${DLFCN_WIN32_SOURCE_DIR}
    BINARY_DIR ${DLFCN_WIN32_BINARY_DIR}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DBUILD_SHARED_LIBS=OFF
               -DBUILD_TESTS=OFF
    BUILD_BYPRODUCTS ${__LIBDL}
  )
  add_dependencies(${DWARFS_PRJ} ${DLFCN_WIN32_PRJ})
endif(WITH_DLFCN_WIN32_BUILD)

# ...................................................................
# Library

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/resources/version.h.in
   ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h
   @ONLY
)

check_symbol_exists(getattrlist "unistd.h" TEBAKO_HAS_GETATTRLIST)
check_symbol_exists(fgetattrlist "unistd.h" TEBAKO_HAS_FGETATTRLIST)

check_symbol_exists(lstat "sys/stat.h" TEBAKO_HAS_LSTAT)
check_symbol_exists(fstatat "sys/stat.h" TEBAKO_HAS_FSTATAT)

check_symbol_exists(opendir "dirent.h" TEBAKO_HAS_OPENDIR)
check_symbol_exists(closedir "dirent.h" TEBAKO_HAS_CLOSEDIR)
check_symbol_exists(readdir "dirent.h" TEBAKO_HAS_READDIR)
check_symbol_exists(seekdir "dirent.h" TEBAKO_HAS_SEEKDIR)
check_symbol_exists(telldir "dirent.h" TEBAKO_HAS_TELLDIR)

check_symbol_exists(dirfd "dirent.h" TEBAKO_HAS_DIRFD)
check_symbol_exists(fdopendir "dirent.h" TEBAKO_HAS_FDOPENDIR)
check_symbol_exists(scandir "dirent.h" TEBAKO_HAS_SCANDIR)

check_symbol_exists(openat "fcntl.h" TEBAKO_HAS_OPENAT)
check_symbol_exists(readv "sys/uio.h" TEBAKO_HAS_READV)

check_symbol_exists(pread "unistd.h" TEBAKO_HAS_PREAD)

check_symbol_exists(flock "sys/file.h" TEBAKO_HAS_FLOCK)


check_cxx_source_compiles(
    "#include <sys/stat.h>
     int main() {
      return mkdir(\"tmp\", 0700);
     }"
    TEBAKO_HAS_POSIX_MKDIR)

check_cxx_source_compiles(
    "#include <direct.h>
     int main() {
      return mkdir(\"tmp\");
     }"
    TEBAKO_HAS_WINDOWS_MKDIR)

if(NOT TEBAKO_HAS_POSIX_MKDIR AND NOT TEBAKO_HAS_WINDOWS_MKDIR)
  message(FATAL_ERROR "Configuration script did not find neither Posix nor Windows mkdir")
endif(NOT TEBAKO_HAS_POSIX_MKDIR AND NOT TEBAKO_HAS_WINDOWS_MKDIR)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/tebako-config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/tebako-config.h
)

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
elseif(IS_WINDOWS)
    add_compile_options(/Zc:__cplusplus /utf-8 /wd4267 /wd4244 /wd5219)
    # Apply /MT or /MTd  (multithread, static version of the run-time library)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${DEPS}/include
                    ${DWARFS_SOURCE_DIR}/folly ${DWARFS_BINARY_DIR}/folly ${DWARFS_SOURCE_DIR}/fbthrift
                    ${DWARFS_BINARY_DIR}/thrift ${DWARFS_BINARY_DIR} ${DWARFS_SOURCE_DIR}/include ${BREW_PREFIX}/include
                    ${DWARFS_BINARY_DIR}/_deps/fmt-src/include
)

add_library(dwarfs-wr STATIC
    "src/file-ctl.cpp"
    "src/dir-ctl.cpp"
    "src/dir-io.cpp"
    "src/file-io.cpp"
    "src/dl-ctl.cpp"
    "src/tebako-io-helpers.cpp"
    "src/tebako-kfd.cpp"
    "src/tebako-mnt.cpp"
    "src/tebako-mfs.cpp"
    "src/tebako-dfs.cpp"
    "src/tebako-fd.cpp"
    "src/tebako-dirent.cpp"
    "include/tebako-common.h"
    "include/tebako-config.h"
    "include/tebako-defines.h"
    "include/tebako-dirent.h"
    "include/tebako-fd.h"
    "include/tebako-io.h"
    "include/tebako-io-inner.h"
    "include/tebako-kfd.h"
    "include/tebako-mnt.h"
    "include/tebako-mfs.h"
    "include/tebako-pch.h"
    "include/tebako-pch-pp.h"
    "include/version.h"
  )

if(RB_W32)
  target_sources(dwarfs-wr PUBLIC
    ""
    "include/tebako-io.h"
    "include/tebako-io-rb-w32-inner.h"
  )
endif(RB_W32)

add_dependencies(dwarfs-wr ${DWARFS_PRJ})
if(WITH_GLOG_BUILD)
  add_dependencies(dwarfs-wr ${GLOG_PRJ})
endif(WITH_GLOG_BUILD)
if(WITH_GFLAGS_BUILD)
  add_dependencies(dwarfs-wr ${GFLAGS_PRJ})
endif(WITH_GFLAGS_BUILD)
if(WITH_FMT_BUILD)
  add_dependencies(dwarfs-wr ${FMT_PRJ})
endif(WITH_FMT_BUILD)
if(WITH_DC_BUILD)
  add_dependencies(dwarfs-wr ${DC_PRJ})
endif(WITH_DC_BUILD)
if(WITH_LZ4_BUILD)
  add_dependencies(dwarfs-wr ${LZ4_PRJ})
endif(WITH_LZ4_BUILD)
if(WITH_JEMALLOC_BUILD)
  add_dependencies(dwarfs-wr ${JEMALLOC_PRJ})
endif(WITH_JEMALLOC_BUILD)
# ...................................................................
# Tests

if(WITH_TESTS)

  add_library(empty MODULE tests/empty/empty.c)
  set_target_properties(empty PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(empty PROPERTIES C_VISIBILITY_PRESET hidden)

  if(WIN32)
    if(DEFINED ENV{TEMP})
      file(TO_CMAKE_PATH "$ENV{TEMP}" TESTS_TMP_DIR)
    elseif(DEFINED ENV{TMP})
      file(TO_CMAKE_PATH "$ENV{TMP}" TESTS_TMP_DIR)
    else()
      message(FATAL_ERROR "Environment variable TEMP or TMP is not defined")
    endif()
    file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" TESTS_USR_DIR)
    find_program(SHELL_PATH cmd.exe)
  else(WIN32)
    # Cannot use env. variables on MacOS because temp folder is unique on each run and
    # it breaks cross-tests on different Os version
    file(TO_CMAKE_PATH "/tmp" TESTS_TMP_DIR)
    file(TO_CMAKE_PATH "/usr" TESTS_USR_DIR)
    find_program(SHELL_PATH sh)
  endif(WIN32)

  get_filename_component(TESTS_BIN_DIR "${SHELL_PATH}" DIRECTORY)
  get_filename_component(TESTS_BIN_NAKED_DIR "${TESTS_BIN_DIR}" NAME)

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/tebako-test-config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-test-config.h
    @ONLY
  )

  if(PREFER_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
    set(GTestMain GTest::Main)
  else(PREFER_SYSTEM_GTEST)
    message(STATUS "Fetching googletest")
    include(FetchContent)

    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        main
    )
# maintain compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# explicitely disable unneeded gmock build
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    set(GTestMain gtest_main)
  endif(PREFER_SYSTEM_GTEST)

  enable_testing()
  include(GoogleTest)
# ...................................................................
# Packaged filesystem
# links for testing are created here to avoid interoperatibility problems Linux/Windows

  add_custom_target(PACKAGED_FILESYSTEM_STEP_1 ALL
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs.cpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${LIBEMPTY_NAME} ${DATA_TEST_DIR}/directory-1
  )

  add_dependencies(PACKAGED_FILESYSTEM_STEP_1 empty)
  add_dependencies(dwarfs-wr PACKAGED_FILESYSTEM_STEP_1)

  set(TESTS_OUTSIDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_files")

  if (WITH_LINK_TESTS)
    add_custom_target(PACKAGED_FILESYSTEM_STEP_2 ALL
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} rm -rf h-link-to-file-2 s-link-to-file-1 s-link-to-dir-1 s-link-outside-of-memfs s-dir-outside-of-memfs
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -f directory-2/file-in-directory-2.txt h-link-to-file-2
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -f -s directory-1/file-in-directory-1.txt s-link-to-file-1
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -f -s directory-2 s-link-to-dir-1
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR}/directory-1 ln -f -s level-2/file-at-level-2.txt s-link-to-level-2
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -f -s ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_files/a-file-outside-of-memfs.txt s-link-outside-of-memfs
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -f -s ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_files s-dir-outside-of-memfs
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -f -s ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_files/a-file-outside-of-memfs.txt
                                                                  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_files/o-link-outside-of-memfs
    )
    add_dependencies(PACKAGED_FILESYSTEM_STEP_2 PACKAGED_FILESYSTEM_STEP_1)

    if (USE_TEMP_FS)
      add_custom_target(PACKAGED_FILESYSTEM_STEP_3 ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filesystem ${DATA_TEST_DIR}
        COMMAND ${GNU_BASH} -c "chmod -w ${DATA_TEST_DIR}/directory-2/file-in-directory-2.txt"
        COMMAND ${MKDWARFS} --force --progress simple -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    else(USE_TEMP_FS)
      add_custom_target(PACKAGED_FILESYSTEM_STEP_3 ALL
        COMMAND ${GNU_BASH} -c "chmod -w ${DATA_TEST_DIR}/directory-2/file-in-directory-2.txt"
        COMMAND ${MKDWARFS} --force --progress simple -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}/*-link-to-* ${DATA_TEST_DIR}/directory-1/*-link-to-*
                                           ${DATA_TEST_DIR}/directory-1/*empty.* ${DATA_TEST_DIR}/*-outside-of-memfs
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    endif(USE_TEMP_FS)
    add_dependencies(PACKAGED_FILESYSTEM_STEP_3 PACKAGED_FILESYSTEM_STEP_2)

  else(WITH_LINK_TESTS)
    if (USE_TEMP_FS)
      add_custom_target(PACKAGED_FILESYSTEM_STEP_3 ALL
        COMMAND ${MKDWARFS} --force --progress simple -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    else(USE_TEMP_FS)
      add_custom_target(PACKAGED_FILESYSTEM_STEP_3 ALL
        COMMAND ${MKDWARFS} --force --progress simple -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}/directory-1/*empty.*
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    endif(USE_TEMP_FS)
    add_dependencies(PACKAGED_FILESYSTEM_STEP_3 PACKAGED_FILESYSTEM_STEP_1)
  endif(WITH_LINK_TESTS)

  add_dependencies(PACKAGED_FILESYSTEM_STEP_3 ${DWARFS_PRJ})

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/resources/tebako-fs.cpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs.cpp
    @ONLY
  )

  if(IS_WINDOWS)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs0.c
      COMMAND ${DEPS}/bin/incbin.exe ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs.cpp -o ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs0.c
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs.cpp ${DATA_BIN_FILE} ${INCBIN_PRJ}
    )
  endif(IS_WINDOWS)

  function(ADD_DWARFS_LIBRARY NAME PRJ APP_NM)
    add_library(${NAME} STATIC IMPORTED)
    set_target_properties(${NAME} PROPERTIES IMPORTED_LOCATION ${_${NAME}})
    add_dependencies(${NAME} ${PRJ})
    target_link_libraries(${APP_NM} ${NAME})
  endfunction()

  function(ADD_DWARFS_LIBRARY_W NAME PRJ APP_NM)
    add_library(${NAME} STATIC IMPORTED)
    set_target_properties(${NAME} PROPERTIES IMPORTED_LOCATION ${_${NAME}})
    add_dependencies(${NAME} ${PRJ})
    target_link_libraries(${APP_NM} "$<LINK_LIBRARY:WHOLE_ARCHIVE,_LIBDWARFS_COMPRESSION>")
  endfunction()

# ...................................................................
# System libraries
# Each library name given to the NAMES option is first considered
# as a library file name and then considered with platform-specific
# prefixes (e.g. lib) and suffixes (e.g. .so). Therefore one may
# specify library file names such as libfoo.a directly. This can be
# used to locate static libraries on UNIX-like systems.
#

  find_package(Boost 1.65 REQUIRED COMPONENTS chrono date_time iostreams filesystem program_options thread system)

  if(IS_WINDOWS)
# For Windwos native build these libraries are installed with vcpkg
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

# __XXXXX - since we are simulating that these libraries were build
# by DWARFS project
    find_library(__LIBZSTD zstd REQUIRED)
    find_library(__LIBARCHIVE archive REQUIRED)
    find_library(__LIBXXHASH xxhash REQUIRED)
    if(CMAKE_BUILD_TYPE STREQUAL Release)
      find_library(__LIBFMT fmt REQUIRED)
    else(CMAKE_BUILD_TYPE STREQUAL Release)
      find_library(__LIBFMT fmtd REQUIRED)
    endif(CMAKE_BUILD_TYPE STREQUAL Release)
    set(_LIBEH "")
else(IS_WINDOWS)
    if (IS_MSYS)
      find_library(__LIBXXHASH "libxxhash.a" REQUIRED)
      if(CMAKE_BUILD_TYPE STREQUAL Release)
        find_library(__LIBFMT "libfmt.a" REQUIRED)
      else(CMAKE_BUILD_TYPE STREQUAL Release)
        find_library(__LIBFMT "libfmtd.a" REQUIRED)
      endif(CMAKE_BUILD_TYPE STREQUAL Release)
      find_library(__LIBZSTD "libzstd.a" REQUIRED)
      set(_LIBEH "")
    else(IS_MSYS)
      set(_LIBEH "gcc_eh")
    endif(IS_MSYS)

    find_library(_LIBEVENT "libevent.a" REQUIRED)
    find_library(_LIBLZMA "liblzma.a" REQUIRED)

# Starting with MacOS BigSur  '/usr/lib' is replaced with a cache of system libraries
# So we cannot just look for libz.a, it is not there anymore
# brew version is keg-only so it cannot be located without a hint

    find_library(_LIBZ "libz.a" REQUIRED HINTS ${DEPS}/lib ${CMAKE_PREFIX_PATH}/opt/zlib/lib)

    find_library(_LIBSSL "libssl.a" REQUIRED HINTS ${DEPS}/lib ${OPENSSL_ROOT_DIR}/lib)
    find_library(_LIBCRYPTO "libcrypto.a" REQUIRED HINTS ${DEPS}/lib ${OPENSSL_ROOT_DIR}/lib)

    if(NOT IS_MSYS)
      find_library(_LIBACL "libacl.a")
    endif()

#  find_library(_LIBPTHREAD "pthread")
#  find_library(_LIBDL "dl")
#  find_library(_LIBC "libc.a")
#  find_library(_LIBM "libm.a")
#  find_library(_LIBRT "librt.a")

    find_library(_LIBUNWIND "libunwind.a")
    find_library(_LIBIBERTY "libiberty.a")

    if (NOT IS_DARWIN)
      add_link_options(-static-libstdc++)
      add_link_options(-static-libgcc)
    endif(NOT IS_DARWIN)

    if (IS_MUSL)
      add_compile_definitions(__musl__)
    endif(IS_MUSL)
  endif(IS_WINDOWS)

  add_executable(wr-bin
    "include/tebako-pch.h"
    "include/tebako-defines.h"
    "include/tebako-io.h"
    "tests/tests-defines-static.c"
    "tests/tebako-fs.cpp"
    "tests/tebako-fs.h"
    "tests/tests.h"
  )

# ...................................................................
# Google test

  file(GLOB TEST_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS tests/tests-*.cpp)

  add_executable(wr-tests
    "include/tebako-pch.h"
    "include/tebako-io.h"
    ${TEST_FILES}
    "tests/tebako-fs.cpp"
    "tests/tebako-fs.h"
    "tests/tests.h"
  )

  add_dependencies(wr-tests PACKAGED_FILESYSTEM_STEP_3)

if(IS_WINDOWS)
  target_sources(wr-bin PUBLIC  "tests/tebako-fs0.c" )
  target_sources(wr-tests PUBLIC "tests/tebako-fs0.c")
endif(IS_WINDOWS)

if(RB_W32)
  target_sources(wr-bin PUBLIC  "tests/rb_w32-tebako-tests-mocks.c")
  target_sources(wr-tests PUBLIC  "tests/rb_w32-tebako-tests-mocks.c")
endif(RB_W32)

# ...................................................................
#  IMPORTANT !!!
#  MUST be in this order otherwise link may fail

  target_link_libraries(wr-bin dwarfs-wr)

  target_compile_options(wr-tests PUBLIC ${GTEST_CFLAGS})
  target_link_libraries(wr-tests dwarfs-wr ${GTEST_LDFLAGS})
  gtest_add_tests(TARGET wr-tests SOURCES ${TEST_FILES})

  add_dependencies(wr-bin ${INCBIN_PRJ} PACKAGED_FILESYSTEM_STEP_3)
  add_dependencies(wr-tests ${INCBIN_PRJ} PACKAGED_FILESYSTEM_STEP_3 wr-bin)

# ...................................................................
# The libraries that are build by dwarfs project
# libdwarfs
# libdwarfs_compression (whole archive needs to be linked)
# libfmt libarchive libfolly libfsst libxxhash
# libmetadata_thrift libthrift_light
  add_dwarfs_library(_LIBDWARFS ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library_w(_LIBDWARFS_COMPRESSION ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBFSST ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBFOLLY ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBT_METADATA ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBT_LIGHT ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBARCHIVE ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBXXHASH ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBZSTD ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBFMT ${DWARFS_PRJ} wr-bin)

  target_link_libraries(wr-tests _LIBDWARFS "$<LINK_LIBRARY:WHOLE_ARCHIVE,_LIBDWARFS_COMPRESSION>"
                        _LIBARCHIVE _LIBFSST   _LIBFOLLY _LIBT_METADATA
                        _LIBT_LIGHT _LIBXXHASH _LIBZSTD  _LIBFMT)

  if(WITH_GLOG_BUILD)
    add_dwarfs_library(_LIBGLOG ${GLOG_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBGLOG)
  else(WITH_GLOG_BUILD)
    list(APPEND _LIBRARIES ${_LIBGLOG})
  endif(WITH_GLOG_BUILD)

  if(WITH_GFLAGS_BUILD)
    add_dwarfs_library(_LIBGFLAGS ${GFLAGS_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBGFLAGS)
  else(WITH_GFLAGS_BUILD)
    list(APPEND _LIBRARIES ${_LIBGFLAGS})
  endif(WITH_GFLAGS_BUILD)

  if(WITH_DC_BUILD)
    add_dwarfs_library(_LIBDC ${DC_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBDC)
  else(WITH_DC_BUILD)
    list(APPEND _LIBRARIES ${_LIBDC})
  endif(WITH_DC_BUILD)

  if(WITH_BROTLI_BUILD)
    add_dwarfs_library(_LIBBROTLIENC ${BROTLI_PRJ} wr-bin)
    add_dwarfs_library(_LIBBROTLIDEC ${BROTLI_PRJ} wr-bin)
    add_dwarfs_library(_LIBBROTLICOMMON ${BROTLI_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBBROTLIENC _LIBBROTLIDEC _LIBBROTLICOMMON)
    elseif(NOT IS_MSYS)
    list(APPEND _LIBRARIES ${_LIBBROTLIENC} ${_LIBBROTLIDEC} ${_LIBBROTLICOMMON})
  endif(WITH_BROTLI_BUILD)

  if(WITH_LZ4_BUILD)
    add_dwarfs_library(_LIBLZ4 ${LZ4_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBLZ4)
  else(WITH_LZ4_BUILD)
    list(APPEND _LIBRARIES ${_LIBLZ4})
  endif(WITH_LZ4_BUILD)

  list(APPEND _LIBRARIES ${_LIBSSL})
  list(APPEND _LIBRARIES ${_LIBCRYPTO})
  list(APPEND _LIBRARIES ${Boost_LIBRARIES} ${_LIBEVENT})

  if(IS_DARWIN)
    list(APPEND _LIBRARIES ${_LIBZ} ${_LIBLZMA} c++abi)
  else(IS_DARWIN)
  # libgcc_eh shall precede libunwind to aboid duplicate symbols during linking
  # liblzma shall follow libunwind because it is libunwind dependency
    list(APPEND _LIBRARIES ${_LIBIBERTY}  ${_LIBACL}    ${_LIBZ}
                           ${_LIBEH}      ${_LIBUNWIND} ${_LIBLZMA}
    )
    if(IS_MSYS)
      find_library(_LIBDL "libdl.a" REQUIRED)
      list(APPEND _LIBRARIES ${_LIBDL})
      target_link_options(wr-bin PUBLIC -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
      target_link_options(wr-tests PUBLIC -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)
    elseif(NOT IS_WINDOWS)
      list(APPEND _LIBRARIES pthread)
      list(APPEND _LIBRARIES dl)
    else()
      list(APPEND _LIBRARIES dl)
    endif(IS_MSYS)
  endif(IS_DARWIN)

  target_link_libraries(wr-bin ${_LIBRARIES})
  target_link_libraries(wr-tests ${GTestMain} ${_LIBRARIES})

  if(WITH_JEMALLOC_BUILD)
    add_dwarfs_library(_LIBJEMALLOC ${JEMALLOC_PRJ} wr-bin)
    target_link_libraries(wr-tests ${_LIBJEMALLOC})
  elseif(WITH_JEMALLOC_BUILD)
    target_link_libraries(wr-bin ${_LIBJEMALLOC})
    target_link_libraries(wr-tests ${_LIBJEMALLOC})
  endif(WITH_JEMALLOC_BUILD)

  if(IS_WINDOWS OR IS_MSYS)
    target_link_libraries(wr-bin crypt32 shlwapi wsock32 ws2_32)
    target_link_libraries(wr-tests crypt32 shlwapi wsock32 ws2_32)
  endif(IS_WINDOWS OR IS_MSYS)

endif(WITH_TESTS)

install(TARGETS
         dwarfs-wr
         DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
        ${__LIBDWARFS}  ${__LIBFSST}   ${__LIBFOLLY} ${__LIBT_METADATA}
        ${__LIBT_LIGHT} ${__LIBXXHASH} ${__LIBZSTD}  ${__LIBARCHIVE}
        ${__LIBDWARFS_COMPRESSION}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if (IS_MSYS)
  install(FILES ${__LIBFMT} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY ${DEPS_INCLUDE_DIR}/fmt DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fmt)
elseif(NOT IS_WINDOWS)
  install(FILES ${__LIBFMT} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY ${DWARFS_BINARY_DIR}/_deps/fmt-src/include DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(IS_MSYS)

if(WITH_GLOG_BUILD)
  install(FILES ${__LIBGLOG} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(WITH_GLOG_BUILD)

if(WITH_GFLAGS_BUILD)
  install(FILES ${__LIBGFLAGS} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(WITH_GFLAGS_BUILD)

if(WITH_DC_BUILD)
  install(FILES ${__LIBDC} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(WITH_DC_BUILD)

if(WITH_BROTLI_BUILD)
  install(FILES ${__LIBBROTLICOMMON} ${__LIBBROTLIENC} ${__LIBBROTLIDEC} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(WITH_BROTLI_BUILD)

if(WITH_LZ4_BUILD)
  install(FILES ${__LIBLZ4} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(WITH_LZ4_BUILD)

if(WITH_JEMALLOC_BUILD)
  install(FILES ${__LIBJEMALLOC} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY ${DEPS_INCLUDE_DIR}/jemalloc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(WITH_JEMALLOC_BUILD)

install(FILES
        "include/tebako-config.h"
        "include/tebako-defines.h"
        "include/tebako-io.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tebako
)

if(RB_W32)
  install(FILES
          "include/tebako-io-rb-w32.h"
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tebako
  )
endif(RB_W32)

install(FILES
        ${DEPS_INCLUDE_DIR}/incbin.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/incbin
)

if(IS_WINDOWS)
  install(FILES
          ${DEPS_BIN_DIR}/incbin.exe
          DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif(IS_WINDOWS)

install(FILES
        ${MKDWARFS}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(IS_WINDOWS OR IS_MSYS)
#  include(InstallRequiredSystemLibraries)

  string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1;\\2;\\3" VERSION_COMPONENTS ${LIBDWARFS_WR_VERSION})

  list(GET VERSION_COMPONENTS 0 VERSION_MAJOR)
  list(GET VERSION_COMPONENTS 1 VERSION_MINOR)
  list(GET VERSION_COMPONENTS 2 VERSION_PATCH)

  set(CPACK_PACKAGE_NAME "libdwarfs-wr")
  set(CPACK_PACKAGE_VENDOR "Ribose Inc.")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tamatebako libdwarfs wrapper package")
  set(CPACK_PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
  set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
  set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "libdwarfs ${CPACK_PACKAGE_VERSION}")

  if (IS_MSYS)
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-mingw-ucrt64")
  else (IS_MSYS)
  endif (IS_MSYS)

  set(CPACK_GENERATOR "7Z")
  include(CPack)
endif(IS_WINDOWS OR IS_MSYS)
