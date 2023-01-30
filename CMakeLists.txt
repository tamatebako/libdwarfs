# Copyright (c) 2021-2023 [Ribose Inc](https://www.ribose.com).
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

cmake_minimum_required (VERSION 3.20)

# For project( ... VERSION ... )
cmake_policy(SET CMP0048 NEW)

# DOWNLOAD_EXTRACT_TIMESTAMP option default = true
if(${CMAKE_VERSION} VERSION_GREATER "3.23.10")
  cmake_policy(SET CMP0135 NEW)
endif()

project(libdwarfs-wr VERSION 0.3.0)

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
include(CheckCSourceCompiles)

# This is the default that will be overwritten for MacOS and MSys
set(GNU_BASH "bash")

include(${CMAKE_SOURCE_DIR}/tools/cmake-scripts/macos-environment.cmake)
include(${CMAKE_SOURCE_DIR}/tools/cmake-scripts/msys-environment.cmake)
include(${CMAKE_SOURCE_DIR}/tools/cmake-scripts/def-external-project.cmake)

# Various locations for external projects
set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps CACHE STRING "Dependencies' folder'")
set(DEPS_INCLUDE_DIR ${DEPS}/include)
set(DEPS_LIB_DIR ${DEPS}/lib)
set(DEPS_BIN_DIR ${DEPS}/bin)
set(DEPS_SBIN_DIR ${DEPS}/sbin)

include_directories(${DEPS_INCLUDE_DIR})
link_directories(${DEPS_LIB_DIR})

if (TEBAKO_BUILD_TARGET)
  if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    if(${TEBAKO_BUILD_TARGET} STREQUAL "arm64-apple-macos11")
      set(JEMALLOC_HOST_OPTION --host=arm-apple-darwin --with-lg-page=16)
      set(HOST_OPTION --host=arm-apple-darwin)
      set(CMAKE_OSX_ARCHITECTURES arm64)
    elseif(${TEBAKO_BUILD_TARGET} STREQUAL "x86_64-apple-macos11")
      set(JEMALLOC_HOST_OPTION --host=x86_64-apple-darwin --with-lg-vaddr=48)
      set(HOST_OPTION --host=x86_64-apple-darwin)
      set(CMAKE_OSX_ARCHITECTURES x86_64)
    else()
      message(FATAL_ERROR "Cannot cross-compile for target ${TEBAKO_BUILD_TARGET}")
    endif()
    message(STATUS "Configuring libdwarfs build script for ${TEBAKO_BUILD_TARGET}")
    add_compile_options(--target=${TEBAKO_BUILD_TARGET})
    add_link_options(--target=${TEBAKO_BUILD_TARGET})
    set(CROSSFLAGS --target=${TEBAKO_BUILD_TARGET})
  else()
    message(${CMAKE_HOST_SYSTEM_NAME})
    message(FATAL_ERROR "Cross-compilation is supported on MacOS only")
  endif()
endif()

# log level
if (NOT TESTS_LOG_LEVEL)
  set(TESTS_LOG_LEVEL "warn")
  message(STATUS "Logging level not specified. Defaulted to warn")
endif()

# coverage
if (WITH_COVERAGE)
  if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Coverage is only supported with the GNU compiler")
  endif()
  add_compile_options(--coverage -O0)
  add_link_options(--coverage)
endif()

# sanitizers
if (WITH_ASAN)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Sanitizers are only supported with the clang compiler")
  endif()
  add_compile_options(-DWITH_ASAN -fsanitize=address,undefined -fno-omit-frame-pointer -fno-common -O1)
  link_libraries(-fsanitize=address,undefined)
endif()

if (WITH_COVERAGE OR WITH_ASAN)
  message(STATUS "Forcing WITH_TEST=ON for code coverage or sanitizers")
  set(WITH_TESTS ON CACHE BOOL "Forced for code coverage or sanitizers" FORCE)
  message(STATUS "Forcing build type to Debug for code coverage or sanitizers")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Forced for code coverage or sanitizers" FORCE)
endif()

if (WITH_TESTS)
  if(MKDWARFS)
    set(TEBAKO_BUILD_SCOPE LIB)
    message(STATUS "Using upstream mkdwarfs at ${MKDWARFS}")
  else(MKDWARFS)
    set(TEBAKO_BUILD_SCOPE MKD)
    set(MKDWARFS ${DEPS_BIN_DIR}/mkdwarfs)
    message(STATUS "Building mkdwarfs at ${MKDWARFS}")
  endif(MKDWARFS)
endif(WITH_TESTS)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Build type was not specified. Defaulted to release")
  set(CMAKE_BUILD_TYPE Release)
endif()

execute_process(
  COMMAND "${GNU_BASH}"
          "-c"
          "echo \$OSTYPE"
  RESULT_VARIABLE OSTYPE_RES
  OUTPUT_VARIABLE OSTYPE_TXT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
)
if(OSTYPE_RES EQUAL 0)
  message(STATUS "OSTYPE: '${OSTYPE_TXT}'")
else(OSTYPE_RES EQUAL 0)
  message(FATAL_ERROR "Failed to detect OSTYPE: ${OSTYPE_TXT}")
endif(OSTYPE_RES EQUAL 0)

# Reasonable defaults
set(USE_JEMALLOC ON)
set(WITH_JEMALLOC_BUILD OFF)
set(WITH_FMT_BUILD OFF)
set(WITH_GLOG_BUILD OFF)
set(WITH_GFLAGS_BUILD OFF)
set(WITH_DC_BUILD OFF)
set(WITH_RUBY_BUILD OFF)

set(PREFER_SYSTEM_GTEST OFF)

set(IS_MUSL OFF)
set(IS_MSYS OFF)
set(IS_DARWIN OFF)

if("${OSTYPE_TXT}" MATCHES "^linux-musl.*")
  set(IS_MUSL ON)
elseif("${OSTYPE_TXT}" MATCHES "^msys*")
  include_directories(${CMAKE_SOURCE_DIR}/tools/include)
  add_compile_definitions(FOLLY_ASSUME_NO_JEMALLOC)
  set(USE_JEMALLOC OFF)
  set(PREFER_SYSTEM_GTEST ON)
  set(WITH_LINK_TESTS OFF)
  set(IS_MSYS ON)
elseif("${OSTYPE_TXT}" MATCHES "^darwin.*")
  message(STATUS "jemalloc homebrew library is not good enough, building locally")
  set(IS_DARWIN ON)
  set(WITH_JEMALLOC_BUILD ON)
endif()

if(NOT ${IS_MSYS})
  set(RB_W32 OFF)
endif(NOT ${IS_MSYS})

message("Configuraion summary:")
message(STATUS "Build scope: ${TEBAKO_BUILD_SCOPE}")
message(STATUS "Building for Ruby Win32: ${RB_W32}")
message(STATUS "Cross-compile options: ${CROSSFLAGS}")
message(STATUS "Generic configure cross-compile options: ${HOST_OPTION}")
message(STATUS "jemalloc configure cross-compile options: ${JEMALLOC_HOST_OPTION}")
message(STATUS "use jemalloc: ${USE_JEMALLOC}")
message(STATUS "CMake OSX architecture flag: ${CMAKE_OSX_ARCHITECTURES}")
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

set(Boost_USE_STATIC_LIBS   ON)
set(Boost_LINK_STATIC       ON)
set(USE_STATIC_DEPS_ON_UNIX ON)
set(GFLAGS_SHARED          OFF)
set(CMAKE_CXX_STANDARD      17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS   OFF)

# ...................................................................
# External projects

def_ext_prj_g(DWARFS "tebako-v0.3.3pre")

if(${WITH_TESTS})
  def_ext_prj_g(INCBIN "348e36b")
  if(${RB_W32})
    message(STATUS "Building Ruby to support unit tests")
    set(WITH_RUBY_BUILD ON)
# Ruby 2.7.7 has build script modification that causes an error (on MSys):
# configure: error: cannot find required auxiliary files: install-sh config.guess config.sub
# change log mentions a need to use autoreconf -i instead of autoconf
# (https://github.com/ruby/ruby/commit/4c7ae840818f995ef776ef99c3ee275d14bd3869)
# but it does not fix the issue
# (It helps to copy missing config.guess and config.sub from autotools manually)
#    def_ext_prj_t(RUBY "2.7.7" "e10127db691d7ff36402cfe88f418c8d025a3f1eea92044b162dd72f0b8c7b90")
    def_ext_prj_t(RUBY "2.7.6" "e7203b0cc09442ed2c08936d483f8ac140ec1c72e37bb5c401646b7866cb5d10")
#    def_ext_prj_t(RUBY "2.7.5" "2755b900a21235b443bb16dadd9032f784d4a88f143d852bc5d154f22b8781f1")
    string(SUBSTRING ${RUBY_VER} 0 3 RUBY_VER_BASE)

    message(STATUS "Building OpenSSL since Ruby 2.x requires 1.1.x but MSys may have 3.x preinstalled")
    set(WITH_OPENSSL_BUILD ON)
    def_ext_prj_g(OPENSSL "OpenSSL_1_1_1s")
  endif(${RB_W32})
endif(${WITH_TESTS})

find_library(_LIBGLOG "libglog.a")
if(${_LIBGLOG} STREQUAL "_LIBGLOG-NOTFOUND")
  message(STATUS "glog static library was not found, building locally")
  set(WITH_GLOG_BUILD ON)
endif()

find_library(_LIBGFLAGS "libgflags.a")
if(${_LIBGFLAGS} STREQUAL "_LIBGFLAGS-NOTFOUND")
  message(STATUS "gflags static library was not found, building locally")
  set(WITH_GFLAGS_BUILD ON)
endif()

if(${IS_MSYS})
  message(STATUS "Building fmt library locally to ensure consistency of MSys linkage")
  set(WITH_FMT_BUILD ON)
else(${IS_MSYS})
  find_library(_LIBFMT "libfmt.a")
  if(${_LIBFMT} STREQUAL "_LIBFMT-NOTFOUND")
    message(STATUS "fmt static library was not found, building locally")
    set(WITH_FMT_BUILD ON)
  endif(${_LIBFMT} STREQUAL "_LIBFMT-NOTFOUND")
endif(${IS_MSYS})

find_library(_LIBDC "libdouble-conversion.a")
if(${_LIBDC} STREQUAL "_LIBDC-NOTFOUND")
  message(STATUS "double-conversion static library was not found, building locally")
  set(WITH_DC_BUILD ON)
endif()

if(${WITH_JEMALLOC_BUILD})
  def_ext_prj_t(JEMALLOC "5.2.1" "34330e5ce276099e2e8950d9335db5a875689a4c6a56751ef3b1d8c537f887f6")
elseif(${USE_JEMALLOC})
  find_library(_LIBJEMALLOC "libjemalloc.a" REQUIRED)
endif()

message("Collecting packages:")
message(STATUS "dwarfs     -  @${DWARFS_TAG}   at  ${DWARFS_SOURCE_DIR}")
if(${WITH_TESTS})
  message(STATUS "incbin     -  @${INCBIN_TAG}  at  ${INCBIN_SOURCE_DIR}")
endif(${WITH_TESTS})
if(${WITH_GLOG_BUILD})
  def_ext_prj_g(GLOG "v0.6.0")
  message(STATUS "glog       -  @${GLOG_TAG}  at  ${GLOG_SOURCE_DIR}")
endif(${WITH_GLOG_BUILD})
if(${WITH_GFLAGS_BUILD})
  def_ext_prj_g(GFLAGS "v2.2.2")
  message(STATUS "gflags     -  @${GFLAGS_TAG}  at  ${GFLAGS_SOURCE_DIR}")
endif(${WITH_GFLAGS_BUILD})
if(${WITH_FMT_BUILD})
  def_ext_prj_g(FMT "8.1.1")
  message(STATUS "fmt        -  @${FMT_TAG}  at  ${FMT_SOURCE_DIR}")
endif(${WITH_FMT_BUILD})
if(${WITH_DC_BUILD})
  def_ext_prj_g(DC "v3.2.0")
  message(STATUS "double-conversion -  @${DC_TAG}  at  ${DC_SOURCE_DIR}")
endif(${WITH_DC_BUILD})
if(${WITH_JEMALLOC_BUILD})
  message(STATUS "jemalloc   -  v${JEMALLOC_VER}  at  ${JEMALLOC_SOURCE_DIR}")
endif(${WITH_JEMALLOC_BUILD})
if(${WITH_RUBY_BUILD})
  message(STATUS "Ruby       -  v${RUBY_VER}  at  ${RUBY_SOURCE_DIR}")
endif(${WITH_RUBY_BUILD})
if(WITH_OPENSSL_BUILD)
  message(STATUS "OpenSSL    -  @${OPENSSL_TAG}  at  ${OPENSSL_SOURCE_DIR}")
endif(WITH_OPENSSL_BUILD)


# ...................................................................
# incbin

if(${WITH_TESTS})
  ExternalProject_Add(${INCBIN_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY https://github.com/graphitemaster/incbin.git
    GIT_TAG ${INCBIN_TAG}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )

# ...................................................................
# Filesystem locations

# DATA_TEST_DIR folder contains raw data for filesystem tests
  if(${USE_TEMP_FS})
    string(RANDOM LENGTH 10 TMPNM)
    set(DATA_TEST_DIR "/tmp/${TMPNM}")
    set(DATA_TEST_DIR_MSG "at '${DATA_TEST_DIR}'")
  else(${USE_TEMP_FS})
    set(DATA_TEST_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filesystem)
    set(DATA_TEST_DIR_MSG "")
  endif(${USE_TEMP_FS})

  if (${WITH_LINK_TESTS})
    set(WLT "with link tests")
    add_compile_definitions(WITH_LINK_TESTS)
  else(${WITH_LINK_TESTS})
    set(WLT "without link tests")
  endif(${WITH_LINK_TESTS})

  message("Using test data set ${WLT} from '${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filesystem' ${DATA_TEST_DIR_MSG}")

# DATA_BIN_DIR folder is used to create packaged filesystem
  set(DATA_BIN_DIR  ${CMAKE_CURRENT_BINARY_DIR})
# DATA_BIN_FILE is packaged filesystem itself
  set(DATA_BIN_FILE ${DATA_BIN_DIR}/fs.bin)

endif(${WITH_TESTS})

# ...................................................................
# DwarFS

# ...................................................................
# The libraries that are build by DwarFS project
# libdwarfs libfolly libfsst libxxhash libmetadata_thrift libthrift_light
# These forward-declarations and BUILD_BYPRODICTS are required to support 'Ninja'
# Otherwise add_dependencies would be enough for 'Unix makefiles' generator

set(__LIBDWARFS "${DWARFS_BINARY_DIR}/libdwarfs.a")
set(__LIBFOLLY "${DWARFS_BINARY_DIR}/folly/libfolly.a")
set(__LIBFSST "${DWARFS_BINARY_DIR}/libfsst.a")
set(__LIBT_METADATA "${DWARFS_BINARY_DIR}/libmetadata_thrift.a")
set(__LIBT_LIGHT "${DWARFS_BINARY_DIR}/libthrift_light.a")
set(__LIBXXHASH "${DWARFS_BINARY_DIR}/libxxhash.a")
set(__LIBZSTD "${DWARFS_BINARY_DIR}/zstd/build/cmake/lib/libzstd.a")
if(IS_MSYS)
  set(__LIBARCHIVE "${DEPS}/lib/libarchive_static.a")
else(IS_MSYS)
  set(__LIBARCHIVE "${DEPS}/lib/libarchive.a")
endif(IS_MSYS)

ExternalProject_Add(${DWARFS_PRJ}
  PREFIX ${DEPS}
  GIT_REPOSITORY https://github.com/tamatebako/dwarfs.git
  GIT_TAG ${DWARFS_TAG}
  UPDATE_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_CXX_STANDARD_REQUIRED=${CMAKE_CXX_STANDARD_REQUIRED}
             -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
             -DWITH_TESTS=OFF
             -DTEBAKO_BUILD_SCOPE=${TEBAKO_BUILD_SCOPE}
             -DTEBAKO_BUILD_TARGET=${TEBAKO_BUILD_TARGET}
             -DDEPS=${DEPS}
             -DCMAKE_PREFIX_PATH=${DEPS}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DTARGET_HOMEBREW=${TARGET_HOMEBREW}
             -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
             -DUSE_JEMALLOC=${USE_JEMALLOC}
  SOURCE_DIR ${DWARFS_SOURCE_DIR}
  BINARY_DIR ${DWARFS_BINARY_DIR}
  BUILD_BYPRODUCTS ${__LIBDWARFS} ${__LIBFSST} ${__LIBFOLLY} ${__LIBT_METADATA}
                   ${__LIBT_LIGHT} ${__LIBXXHASH} ${__LIBZSTD} ${__LIBARCHIVE}
)

# ...................................................................
# glog

if(${WITH_GLOG_BUILD})
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
               -DCMAKE_CXX_FLAGS=${CROSSFLAGS}
               -DCMAKE_EXE_LINKER_FLAGS=${CROSSFLAGS}
               -DCMAKE_SHARED_LINKER_FLAGS=${CROSSFLAGS}
               -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    SOURCE_DIR ${GLOG_SOURCE_DIR}
    BINARY_DIR ${GLOG_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBGLOG}
  )
  add_dependencies(${DWARFS_PRJ} ${GLOG_PRJ})
endif(${WITH_GLOG_BUILD})

# ...................................................................
# gflags

if(${WITH_GFLAGS_BUILD})
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
               -DCMAKE_CXX_FLAGS=${CROSSFLAGS}
               -DCMAKE_EXE_LINKER_FLAGS=${CROSSFLAGS}
               -DCMAKE_SHARED_LINKER_FLAGS=${CROSSFLAGS}
               -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    SOURCE_DIR ${GFLAGS_SOURCE_DIR}
    BINARY_DIR ${GFLAGS_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBGFLAGS}
  )
  add_dependencies(${GLOG_PRJ} ${GFLAGS_PRJ})
  add_dependencies(${DWARFS_PRJ} ${GFLAGS_PRJ})
endif(${WITH_GFLAGS_BUILD})

# ...................................................................
# fmt

if(${WITH_FMT_BUILD})
  set(__LIBFMT "${DEPS}/lib/libfmt.a")
  ExternalProject_Add(${FMT_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
    GIT_TAG ${FMT_TAG}
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DCMAKE_CXX_FLAGS=${CROSSFLAGS}
               -DCMAKE_EXE_LINKER_FLAGS=${CROSSFLAGS}
               -DCMAKE_SHARED_LINKER_FLAGS=${CROSSFLAGS}
               -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
               -DFMT_TEST=OFF
    SOURCE_DIR ${FMT_SOURCE_DIR}
    BINARY_DIR ${FMT_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBFMT}
  )
  add_dependencies(${DWARFS_PRJ} ${FMT_PRJ})
endif(${WITH_FMT_BUILD})

# ...................................................................
# double-conversion

if(${WITH_DC_BUILD})
  set(__LIBDC "${DEPS}/lib/libdouble-conversion.a")
  ExternalProject_Add(${DC_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/google/double-conversion.git"
    GIT_TAG ${DC_TAG}
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DCMAKE_CXX_FLAGS=${CROSSFLAGS}
               -DCMAKE_EXE_LINKER_FLAGS=${CROSSFLAGS}
               -DCMAKE_SHARED_LINKER_FLAGS=${CROSSFLAGS}
               -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    SOURCE_DIR ${DC_SOURCE_DIR}
    BINARY_DIR ${DC_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBDC}
  )
  add_dependencies(${DWARFS_PRJ} ${DC_PRJ})
endif(${WITH_DC_BUILD})

# ...................................................................
# jemalloc

if(${WITH_JEMALLOC_BUILD})
  set(__LIBJEMALLOC "${DEPS}/lib/libjemalloc.a")

  ExternalProject_Add(${JEMALLOC_PRJ}
    PREFIX ${DEPS}
    URL https://github.com/jemalloc/jemalloc/releases/download/${JEMALLOC_VER}/jemalloc-${JEMALLOC_VER}.tar.bz2
    URL_HASH SHA256=${JEMALLOC_HASH}
    DOWNLOAD_NO_PROGRESS true
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env CFLAGS=${CROSSFLAGS}  CXXFLAGS=${CROSSFLAGS}
                      ${JEMALLOC_SOURCE_DIR}/configure
                      ${JEMALLOC_HOST_OPTION}
                      --srcdir=${JEMALLOC_SOURCE_DIR}
                      --prefix=${DEPS}
                      --enable-static
                      --disable-shared
                      --datarootdir=${DEPS}/share.dummy
                      --disable-syscall
    SOURCE_DIR ${JEMALLOC_SOURCE_DIR}
    BINARY_DIR ${JEMALLOC_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBJEMALLOC}
  )
endif(${WITH_JEMALLOC_BUILD})

# ...................................................................
# OpenSSL
if(WITH_OPENSSL_BUILD)
  set(__LIBSSL "${DEPS}/lib/libssl.a")
  set(__LIBCRYPTO "${DEPS}/lib/libcrypto.a")

  ExternalProject_Add(${OPENSSL_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY "https://github.com/openssl/openssl.git"
    GIT_TAG ${OPENSSL_TAG}
    UPDATE_COMMAND ""
    SOURCE_DIR ${OPENSSL_SOURCE_DIR}
    BINARY_DIR ${OPENSSL_BINARY_DIR}
    CONFIGURE_COMMAND   ${GNU_BASH} -c "${OPENSSL_SOURCE_DIR}/config          \
                                                        --openssldir=${DEPS}  \
                                                        --prefix=${DEPS}"
    BUILD_BYPRODUCTS ${__LIBSSL} ${__LIBCRYPTO}
  )
  add_dependencies(${DWARFS_PRJ} ${OPENSSL_PRJ})
endif(WITH_OPENSSL_BUILD)

# ...................................................................
# Ruby
# Notes:
# -- we build ruby for certain MSys tests only, so MSys configuration
#    is hardcoded (GNUmakefile and probably more)
# -- there are some strange issues with autoconf. It looks like aclocal.m4
#    is written after configure and it causes reconf when make is called
#    In order to fix it I do 'touch' for configure though it does not look good

if(${WITH_RUBY_BUILD})
  set(__LIBRUBY "${RUBY_BINARY_DIR}/libx64-msvcrt-ruby270-static.a")

  ExternalProject_Add(${RUBY_PRJ}
    PREFIX ${DEPS}
    URL https://cache.ruby-lang.org/pub/ruby/${RUBY_VER_BASE}/ruby-${RUBY_VER}.tar.gz
    URL_HASH SHA256=${RUBY_HASH}
    DOWNLOAD_NO_PROGRESS true
    SOURCE_DIR ${RUBY_SOURCE_DIR}
    BINARY_DIR ${RUBY_BINARY_DIR}
    CONFIGURE_COMMAND ${GNU_BASH} -c "${RUBY_SOURCE_DIR}/configure                            \
                                                        --enable-bundled-libyaml              \
                                                        --without-gmp                         \
                                                        --disable-dtrace                      \
                                                        --enable-debug-env                    \
                                                        --disable-install-doc                 \
                                                        --with-static-linked-ext              \
                                                        --with-out-ext=syslog,pty,dbm,gdbm    \
                                                        --srcdir=${RUBY_SOURCE_DIR}           \
                                                        --prefix=${DEPS}                      \
                                                        --with-openssl-dir=${DEPS}"
    BUILD_COMMAND    ${GNU_BASH} -c "touch ${RUBY_SOURCE_DIR}/configure"
    INSTALL_COMMAND  ${GNU_BASH} -c "make -f GNUmakefile install"
    BUILD_BYPRODUCTS ${__LIBRUBY}
  )
  if(WITH_OPENSSL_BUILD)
    add_dependencies(${RUBY_PRJ} ${OPENSSL_PRJ})
  endif(WITH_OPENSSL_BUILD)
endif(${WITH_RUBY_BUILD})

# ...................................................................
# Library

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/resources/version.h.in
   ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h
   @ONLY
)

check_symbol_exists(O_DIRECTORY "fcntl.h" TEBAKO_HAS_O_DIRECTORY)
check_symbol_exists(O_NOFOLLOW "fcntl.h" TEBAKO_HAS_O_NOFOLLOW)
check_symbol_exists(O_BINARY "fcntl.h" TEBAKO_HAS_O_BINARY)

check_symbol_exists(S_ISLNK "sys/stat.h" TEBAKO_HAS_S_ISLNK)

check_symbol_exists(getattrlist "unistd.h" TEBAKO_HAS_GETATTRLIST)
check_symbol_exists(fgetattrlist "unistd.h" TEBAKO_HAS_FGETATTRLIST)

check_symbol_exists(lstat "sys/stat.h" TEBAKO_HAS_LSTAT)
check_symbol_exists(fstatat "sys/stat.h" TEBAKO_HAS_FSTATAT)

check_symbol_exists(dirfd "dirent.h" TEBAKO_HAS_DIRFD)
check_symbol_exists(fdopendir "dirent.h" TEBAKO_HAS_FDOPENDIR)
check_symbol_exists(scandir "dirent.h" TEBAKO_HAS_SCANDIR)

check_symbol_exists(openat "fcntl.h" TEBAKO_HAS_OPENAT)
check_symbol_exists(readv "sys/uio.h" TEBAKO_HAS_READV)

check_symbol_exists(pread "unistd.h" TEBAKO_HAS_PREAD)

check_c_source_compiles(
    "#include <sys/stat.h>
     int main() {
      return mkdir(\"tmp\", 0700);
     }"
    TEBAKO_HAS_POSIX_MKDIR)

check_c_source_compiles(
    "#include <sys/stat.h>
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

set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2 -g")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${INCBIN_SOURCE_DIR}
                    ${DWARFS_SOURCE_DIR}/folly ${DWARFS_BINARY_DIR}/folly ${DWARFS_SOURCE_DIR}/fbthrift
                    ${DWARFS_BINARY_DIR}/thrift ${DWARFS_BINARY_DIR} ${DWARFS_SOURCE_DIR}/include ${BREW_PREFIX}/include
)

add_library(dwarfs-wr STATIC
    "src/file-ctl.cpp"
    "src/dir-ctl.cpp"
    "src/dir-io.cpp"
    "src/file-io.cpp"
    "src/dl.cpp"
    "src/tebako-io-helpers.cpp"
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
    "include/tebako-kdf.h"
    "include/tebako-mfs.h"
    "include/tebako-pch.h"
    "include/tebako-pch-pp.h"
    "include/version.h"
  )

if(${RB_W32})
  target_sources(dwarfs-wr PUBLIC
    "include/tebako-io-rb-w32.h"
    "include/tebako-io-rb-w32-inner.h"
  )
  add_compile_definitions(RB_W32)
endif(${RB_W32})

add_dependencies(dwarfs-wr ${DWARFS_PRJ})
if(${WITH_GLOG_BUILD})
  add_dependencies(dwarfs-wr ${GLOG_PRJ})
endif(${WITH_GLOG_BUILD})
if(${WITH_GFLAGS_BUILD})
  add_dependencies(dwarfs-wr ${GFLAGS_PRJ})
endif(${WITH_GFLAGS_BUILD})
if(${WITH_FMT_BUILD})
  add_dependencies(dwarfs-wr ${FMT_PRJ})
endif(${WITH_FMT_BUILD})
if(${WITH_DC_BUILD})
  add_dependencies(dwarfs-wr ${DC_PRJ})
endif(${WITH_DC_BUILD})
if(${WITH_JEMALLOC_BUILD})
  add_dependencies(dwarfs-wr ${JEMALLOC_PRJ})
endif(${WITH_JEMALLOC_BUILD})
# ...................................................................
# Tests

if(${WITH_TESTS})

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/tebako-test-config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-test-config.h
    @ONLY
  )

  if(NOT ${PREFER_SYSTEM_GTEST})
    message("Fetching googletest")
    include(FetchContent)

    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        release-1.11.0
    )
# maintain compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# explicitely disable unneeded gmock build
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
  endif(NOT ${PREFER_SYSTEM_GTEST})

  enable_testing()
  include(GoogleTest)
# ...................................................................
# Packaged filesystem
# links for testing are created here to avoid interoperatibility problems Linux/Windows

  add_library(empty MODULE tests/empty/empty.c)
  set_target_properties(empty PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(empty PROPERTIES C_VISIBILITY_PRESET hidden)

  add_custom_target(PACKAGED_FILESYSTEM_STEP_1 ALL
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs.cpp
    COMMAND ${GNU_BASH} -c "cp ${CMAKE_CURRENT_BINARY_DIR}/*empty.* ${DATA_TEST_DIR}/directory-1"
  )

  add_dependencies(PACKAGED_FILESYSTEM_STEP_1 empty)
  add_dependencies(dwarfs-wr PACKAGED_FILESYSTEM_STEP_1)

  if (${WITH_LINK_TESTS})
    add_custom_target(PACKAGED_FILESYSTEM_STEP_2 ALL
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} rm -rf h-link-to-file-2 s-link-to-file-1 s-link-to-dir-1 s-link-outside-of-memfs s-dir-outside-of-memfs
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln directory-2/file-in-directory-2.txt h-link-to-file-2
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -s directory-1/file-in-directory-1.txt s-link-to-file-1
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -s directory-2 s-link-to-dir-1
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -s ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_files/a-file-outside-of-memfs.txt s-link-outside-of-memfs
      COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -s ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_files s-dir-outside-of-memfs
    )
    add_dependencies(PACKAGED_FILESYSTEM_STEP_2 PACKAGED_FILESYSTEM_STEP_1)

    if (${USE_TEMP_FS})
      add_custom_target(PACKAGED_FILESYSTEM_STEP_3 ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filesystem ${DATA_TEST_DIR}
        COMMAND ${MKDWARFS} -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    else(${USE_TEMP_FS})
      add_custom_target(PACKAGED_FILESYSTEM_STEP_3 ALL
        COMMAND ${MKDWARFS} -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}/*-link-to-* ${DATA_TEST_DIR}/directory-1/*empty.* ${DATA_TEST_DIR}/*-outside-of-memfs
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    endif(${USE_TEMP_FS})
    add_dependencies(PACKAGED_FILESYSTEM_STEP_3 PACKAGED_FILESYSTEM_STEP_2)

  else(${WITH_LINK_TESTS})
    if (${USE_TEMP_FS})
      add_custom_target(PACKAGED_FILESYSTEM_STEP_3 ALL
        COMMAND ${MKDWARFS} -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    else(${USE_TEMP_FS})
      add_custom_target(PACKAGED_FILESYSTEM_STEP_3 ALL
        COMMAND ${MKDWARFS} -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}/directory-1/*empty.*
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    endif(${USE_TEMP_FS})
    add_dependencies(PACKAGED_FILESYSTEM_STEP_3 PACKAGED_FILESYSTEM_STEP_1)
  endif(${WITH_LINK_TESTS})

  add_dependencies(PACKAGED_FILESYSTEM_STEP_3 ${DWARFS_PRJ})

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/resources/tebako-fs.cpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs.cpp
    @ONLY
  )

  function(ADD_DWARFS_LIBRARY NAME PRJ APP_NM)
    add_library(${NAME} STATIC IMPORTED)
    set_target_properties(${NAME} PROPERTIES IMPORTED_LOCATION ${_${NAME}})
    add_dependencies(${NAME} ${PRJ})
    target_link_libraries(${APP_NM} ${NAME})
  endfunction()

# ...................................................................
# System libraries
# Each library name given to the NAMES option is first considered
# as a library file name and then considered with platform-specific
# prefixes (e.g. lib) and suffixes (e.g. .so). Therefore one may
# specify library file names such as libfoo.a directly. This can be
# used to locate static libraries on UNIX-like systems.
#

  find_package(Boost 1.65 REQUIRED COMPONENTS date_time filesystem program_options system)

  find_library(_LIBEVENT "libevent.a" REQUIRED)
  find_library(_LIBLZ4 "liblz4.a" REQUIRED)
  find_library(_LIBLZMA "liblzma.a" REQUIRED)
  find_library(_LIBZ "libz.a" REQUIRED)

  if (NOT WITH_OPENSSL_BUILD)
    find_library(_LIBSSL "libssl.a" REQUIRED)
    find_library(_LIBCRYPTO "libcrypto.a" REQUIRED)
  endif (NOT WITH_OPENSSL_BUILD)

  if (NOT IS_MSYS)
    find_library(_LIBACL "libacl.a")
  endif()

  find_library(_LIBPTHREAD "pthread")
  find_library(_LIBDL "dl")
  find_library(_LIBC "libc.a")
  find_library(_LIBM "libm.a")
  find_library(_LIBRT "librt.a")
  find_library(_LIBUNWIND "libunwind.a")
  find_library(_LIBIBERTY "libiberty.a")

  if (IS_MSYS)
    add_link_options(-static-libstdc++ -static)
  elseif (NOT IS_DARWIN)
    add_link_options(-static-libstdc++)
  endif()

  if (IS_MUSL)
    add_compile_definitions(__musl__)
  endif(IS_MUSL)

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

  add_executable(wr-tests
    "include/tebako-pch.h"
    "include/tebako-io.h"
    "tests/tests-fs-load.cpp"
    "tests/tests-file-ctl.cpp"
    "tests/tests-file-io.cpp"
    "tests/tests-dir-ctl.cpp"
    "tests/tests-ln.cpp"
    "tests/tests-dl.cpp"
    "tests/tebako-fs.cpp"
    "tests/tebako-fs.h"
    "tests/tests.h"
  )

if(${RB_W32})
  target_sources(wr-bin PUBLIC "tests/tests-init-rb-w32.c" "tests/tests-init-rb-w32.h")
  target_sources(wr-tests PUBLIC "tests/tests-dir-io-rb-w32.cpp")
  target_sources(wr-tests PUBLIC "tests/tests-init-rb-w32.c" "tests/tests-init-rb-w32.h")
else(${RB_W32})
  target_sources(wr-tests PUBLIC "tests/tests-dir-io.cpp")
endif(${RB_W32})

# ...................................................................
#  IMPORTANT !!!
#  MUST be in this order otherwise link may fail

  target_link_libraries(wr-bin dwarfs-wr)

  target_compile_options(wr-tests PUBLIC ${GTEST_CFLAGS})
  target_link_libraries(wr-tests dwarfs-wr ${GTEST_LDFLAGS})
  gtest_add_tests(TARGET wr-tests)

  add_dependencies(wr-bin ${INCBIN_PRJ} PACKAGED_FILESYSTEM_STEP_3)
  add_dependencies(wr-tests ${INCBIN_PRJ} PACKAGED_FILESYSTEM_STEP_3 wr-bin)

# ...................................................................
# The libraries that are build by dwarfs project
# libdwarfs libfolly libfsst libxxhash libmetadata_thrift libthrift_light
  add_dwarfs_library(_LIBDWARFS ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBFSST ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBFOLLY ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBT_METADATA ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBT_LIGHT ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBXXHASH ${DWARFS_PRJ} wr-bin)
  add_dwarfs_library(_LIBZSTD ${DWARFS_PRJ} wr-bin)

# ...................................................................
#  libarchive

  add_dwarfs_library(_LIBARCHIVE ${DWARFS_PRJ} wr-bin)

  target_link_libraries(wr-tests _LIBDWARFS _LIBFSST   _LIBFOLLY _LIBT_METADATA
                                 _LIBT_LIGHT _LIBXXHASH _LIBZSTD  _LIBARCHIVE)

  if(${WITH_GLOG_BUILD})
    add_dwarfs_library(_LIBGLOG ${GLOG_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBGLOG)
  else(${WITH_GLOG_BUILD})
    list(APPEND _LIBRARIES ${_LIBGLOG})
  endif(${WITH_GLOG_BUILD})

  if(${WITH_GFLAGS_BUILD})
    add_dwarfs_library(_LIBGFLAGS ${GFLAGS_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBGFLAGS)
  else(${WITH_GFLAGS_BUILD})
    list(APPEND _LIBRARIES ${_LIBGFLAGS})
  endif(${WITH_GFLAGS_BUILD})

  if(${WITH_FMT_BUILD})
    add_dwarfs_library(_LIBFMT ${FMT_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBFMT)
  else(${WITH_FMT_BUILD})
    list(APPEND _LIBRARIES ${_LIBFMT})
  endif(${WITH_FMT_BUILD})

  if(${WITH_DC_BUILD})
    add_dwarfs_library(_LIBDC ${DC_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBDC)
  else(${WITH_DC_BUILD})
    list(APPEND _LIBRARIES ${_LIBDC})
  endif(${WITH_DC_BUILD})

  if(WITH_OPENSSL_BUILD)
    add_dwarfs_library(_LIBSSL ${OPENSSL_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBSSL)
    add_dwarfs_library(_LIBCRYPTO ${OPENSSL_PRJ} wr-bin)
    target_link_libraries(wr-tests _LIBCRYPTO)
  else(WITH_OPENSSL_BUILD)
    list(APPEND _LIBRARIES ${_LIBSSL})
    list(APPEND _LIBRARIES ${_LIBCRYPTO})
  endif(WITH_OPENSSL_BUILD)

  list(APPEND _LIBRARIES ${Boost_LIBRARIES} ${_LIBEVENT})

  if(IS_DARWIN)
    list(APPEND _LIBRARIES ${_LIBLZ4} ${_LIBZ} ${_LIBLZMA})
  else(IS_DARWIN)
    if(WITH_ASAN)
      list(APPEND _LIBRARIES ${_LIBIBERTY} ${_LIBACL}      ${_LIBLZ4}
                             ${_LIBZ}      ${_LIBUNWIND}   ${_LIBLZMA}
      )
    else(WITH_ASAN)
      list(APPEND _LIBRARIES ${_LIBIBERTY} ${_LIBACL}      ${_LIBLZ4}
                             ${_LIBZ}      ${_LIBUNWIND}   ${_LIBLZMA}
      )
      if(NOT ${RB_W32})
        list(APPEND _LIBRARIES pthread)
      endif(NOT ${RB_W32})
    endif(WITH_ASAN)
    list(APPEND _LIBRARIES dl)
  endif(IS_DARWIN)

  target_link_libraries(wr-bin ${_LIBRARIES})
  target_link_libraries(wr-tests gtest_main gtest ${_LIBRARIES})

  if(${WITH_JEMALLOC_BUILD})
    add_dwarfs_library(_LIBJEMALLOC ${JEMALLOC_PRJ} wr-bin)
    target_link_libraries(wr-tests ${_LIBJEMALLOC})
  elseif(${USE_JEMALLOC})
    target_link_libraries(wr-bin ${_LIBJEMALLOC})
    target_link_libraries(wr-tests ${_LIBJEMALLOC})
  endif()

  if(${WITH_RUBY_BUILD})
    add_dwarfs_library(_LIBRUBY ${RUBY_PRJ} wr-bin)
    target_link_libraries(wr-bin imagehlp iphlpapi)
    target_link_libraries(wr-tests ${__LIBRUBY} imagehlp iphlpapi)
  endif(${WITH_RUBY_BUILD})

  if(${IS_MSYS})
    target_link_libraries(wr-bin shlwapi wsock32 ws2_32)
    target_link_libraries(wr-tests shlwapi wsock32 ws2_32)
  endif(${IS_MSYS})

endif(${WITH_TESTS})

install(TARGETS
         dwarfs-wr
         DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
        ${__LIBDWARFS}  ${__LIBFSST}   ${__LIBFOLLY} ${__LIBT_METADATA}
        ${__LIBT_LIGHT} ${__LIBXXHASH} ${__LIBZSTD}  ${__LIBARCHIVE}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(${WITH_GLOG_BUILD})
  install(FILES ${__LIBGLOG} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(${WITH_GLOG_BUILD})

if(${WITH_GFLAGS_BUILD})
  install(FILES ${__LIBGFLAGS} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(${WITH_GFLAGS_BUILD})

if(${WITH_FMT_BUILD})
  install(FILES ${__LIBFMT} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(${WITH_FMT_BUILD})

if(${WITH_DC_BUILD})
  install(FILES ${__LIBDC} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(${WITH_DC_BUILD})

if(${WITH_JEMALLOC_BUILD})
  install(FILES ${__LIBJEMALLOC} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY ${DEPS_INCLUDE_DIR}/jemalloc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(${WITH_JEMALLOC_BUILD})

install(FILES
        "include/tebako-config.h"
        "include/tebako-defines.h"
        "include/tebako-io.h"
        "include/tebako-io-rb-w32.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tebako
)

if(IS_MSYS)
  install(FILES
          ${DEPS_BIN_DIR}/mkdwarfs.exe
          DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
else(IS_MSYS)
  install(FILES
          ${DEPS_BIN_DIR}/mkdwarfs
          DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif(IS_MSYS)
