# Copyright (c) 2021, [Ribose Inc](https://www.ribose.com).
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

cmake_policy(SET CMP0048 NEW)
# For project( ... VERSION ... )

project(libdwarfs-wr VERSION 0.1.5)

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
include(${CMAKE_SOURCE_DIR}/cmake/def_ext_prj.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/def_darwin_env.cmake)

set(Boost_USE_STATIC_LIBS   ON)
set(Boost_LINK_STATIC       ON)
set(USE_STATIC_DEPS_ON_UNIX ON)
set(GFLAGS_SHARED          OFF)
set(CMAKE_CXX_STANDARD      17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS   OFF)
set(DWARFS_CXX_FLAGS       "")

# Various locations for external projects
set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps CACHE STRING "Dependencies' folder'")
set(DEPS_INCLUDE_DIR ${DEPS}/include)
set(DEPS_LIB_DIR ${DEPS}/lib)
set(DEPS_BIN_DIR ${DEPS}/bin)
set(DEPS_SBIN_DIR ${DEPS}/sbin)

# ...................................................................
# External projects

def_ext_prj_g(DWARFS "libdwarfs_wr_v0.1.6")
def_ext_prj_g(LIBARCHIVE "v3.5.1")


if(${WITH_TESTS})
  def_ext_prj_g(INCBIN "348e36b")
endif(${WITH_TESTS})

find_library(_LIBGLOG "libglog.a")
if(${_LIBGLOG} STREQUAL "_LIBGLOG-NOTFOUND")
  message("glog static library was not found, building locally")
  set(WITH_GLOG_BUILD ON)
  def_ext_prj_g(GLOG "v0.5.0")
else()
  set(WITH_GLOG_BUILD OFF)
endif()

find_library(_LIBGFLAGS "libgflags.a")
if(${_LIBGFLAGS} STREQUAL "_LIBGFLAGS-NOTFOUND")
  message("gflags static library was not found, building locally")
  set(WITH_GFLAGS_BUILD ON)
  def_ext_prj_g(GFLAGS "v2.2.2")
else()
  set(WITH_GFLAGS_BUILD OFF)
endif()

message("Collecting packages:")
message("dwarfs     - " @${DWARFS_TAG}  " at " ${DWARFS_SOURCE_DIR})
message("libarchive - " @${LIBARCHIVE_TAG} " at " ${LIBARCHIVE_SOURCE_DIR})
if(${WITH_TESTS})
  message("incbin     - " @${INCBIN_TAG} " at " ${INCBIN_SOURCE_DIR})
endif(${WITH_TESTS})
if(${WITH_GLOG_BUILD})
  message("glog       - " @${GLOG_TAG} " at " ${GLOG_SOURCE_DIR})
endif(${WITH_GLOG_BUILD})
if(${WITH_GFLAGS_BUILD})
  message("gflags     - " @${GFLAGS_TAG} " at " ${GFLAGS_SOURCE_DIR})
endif(${WITH_GFLAGS_BUILD})

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
    add_compile_definitions("WITH_LINK_TESTS")
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
set(__LIBARCHIVE "${DEPS}/lib/libarchive.a")

# Static build for dwarfs core is broken again
# It is mot an issue any way since we agreed to limited set of shared libraries
# as described here https://github.com/tamatebako/tebako/issues/42
# We do static linkage within additional tests anyway.
# Practically it is possible to build it static, th eissue is related to
# dwarfs build scripts
# Newer version of dwarfs has new issues
# https://github.com/tamatebako/libdwarfs/issues/43

ExternalProject_Add(${DWARFS_PRJ}
  PREFIX ${DEPS}
  GIT_REPOSITORY https://github.com/tamatebako/dwarfs.git
  GIT_TAG ${DWARFS_TAG}
  UPDATE_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
             -DCMAKE_CXX_STANDARD=14
             -DCMAKE_CXX_STANDARD_REQUIRED=ON 
             -DCMAKE_CXX_EXTENSIONS=OFF
  SOURCE_DIR ${DWARFS_SOURCE_DIR}
  BINARY_DIR ${DWARFS_BINARY_DIR}
  BUILD_BYPRODUCTS ${__LIBDWARFS} ${__LIBFSST} ${__LIBFOLLY} ${__LIBT_METADATA}
                   ${__LIBT_LIGHT} ${__LIBXXHASH} ${__LIBZSTD}
)

ExternalProject_Add(${LIBARCHIVE_PRJ}
  PREFIX "${DEPS}"
  GIT_REPOSITORY "https://github.com/libarchive/libarchive.git"
  GIT_TAG ${LIBARCHIVE_TAG}
  UPDATE_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS}
             -DCMAKE_BUILD_TYPE=Release
             -DENABLE_ICONV:BOOL=OFF
             -DENABLE_LIBXML2:BOOL=OFF
             -DENABLE_TEST:BOOL=OFF
  SOURCE_DIR ${LIBARCHIVE_SOURCE_DIR}
  BINARY_DIR ${LIBARCHIVE_BINARY_DIR}
  BUILD_BYPRODUCTS ${__LIBARCHIVE}
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
               -DWITH_GTEST:BOOL=OFF
               -DCMAKE_CXX_STANDARD=17
               -DCMAKE_CXX_STANDARD_REQUIRED=ON
               -DCMAKE_CXX_EXTENSIONS=OFF
    SOURCE_DIR ${GLOG_SOURCE_DIR}
    BINARY_DIR ${GLOG_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBGLOG}
  )
endif(${WITH_GLOG_BUILD})

# ...................................................................
# glog

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
    SOURCE_DIR ${GFLAGS_SOURCE_DIR}
    BINARY_DIR ${GFLAGS_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBGFLAGS}
  )
endif(${WITH_GFLAGS_BUILD})

# ...................................................................
# Library

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/resources/version.h.in
   ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h
   @ONLY
)


set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2 -g")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${DEPS}/include
                    ${INCBIN_SOURCE_DIR}
                    ${DWARFS_SOURCE_DIR}/folly ${DWARFS_BINARY_DIR}/folly ${DWARFS_SOURCE_DIR}/fbthrift
                    ${DWARFS_BINARY_DIR}/thrift ${DWARFS_BINARY_DIR} ${DWARFS_SOURCE_DIR}/include ${BREW_PREFIX}/include
)

add_library(dwarfs-wr STATIC
    "src/file-ctl.cpp"
    "src/dir-ctl.cpp"
    "src/file-io.cpp"
    "src/dir-io.cpp"
    "src/dl.cpp"
    "src/tebako-io-helpers.cpp"
    "src/tebako-mfs.cpp"
    "src/tebako-dfs.cpp"
    "src/tebako-fd.cpp"
    "src/tebako-dirent.cpp"
    "include/tebako-pch.h"
    "include/tebako-pch-pp.h"
    "include/tebako-common.h"
    "include/tebako-io.h"
    "include/tebako-io-inner.h"
    "include/tebako-fd.h"
    "include/tebako-dirent.h"
    "include/tebako-mfs.h"
    "include/tebako-defines.h"
    "include/version.h"
  )

add_dependencies(dwarfs-wr ${DWARFS_PRJ} ${LIBARCHIVE_PRJ})
if(${WITH_GLOG_BUILD})
  add_dependencies(dwarfs-wr ${GLOG_PRJ})
endif(${WITH_GLOG_BUILD})
if(${WITH_GFLAGS_BUILD})
  add_dependencies(dwarfs-wr ${GFLAGS_PRJ})
endif(${WITH_GFLAGS_BUILD})

# ...................................................................
# Tests

if(${WITH_TESTS})
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
  enable_testing()
  include(GoogleTest)

# ...................................................................
# Packaged filesystem
# links for testing are created here to avoid interoperatibility problems Linux/Windows

  add_library(empty MODULE tests/empty/empty.c)
  set_target_properties(empty PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(empty PROPERTIES C_VISIBILITY_PRESET hidden)

  add_custom_target(PREPACKAGED_FILESYSTEM ALL
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/tests/tebako-fs.cpp
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filesystem ${DATA_TEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/*empty.* ${DATA_TEST_DIR}/directory-1
  )

  add_dependencies(PREPACKAGED_FILESYSTEM empty)

  if (${WITH_LINK_TESTS})
    if (${USE_TEMP_FS})
      add_custom_target(PACKAGED_FILESYSTEM ALL
        COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} rm -rf h-link-to-file-2 s-link-to-file-1 s-link-to-dir-1
        COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln directory-2/file-in-directory-2.txt h-link-to-file-2
        COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -s directory-1/file-in-directory-1.txt s-link-to-file-1
        COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -s directory-2 s-link-to-dir-1
        COMMAND ${DEPS_BIN_DIR}/mkdwarfs -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    else(${USE_TEMP_FS})
      add_custom_target(PACKAGED_FILESYSTEM ALL
        COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} rm -rf h-link-to-file-2 s-link-to-file-1 s-link-to-dir-1
        COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln directory-2/file-in-directory-2.txt h-link-to-file-2
        COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -s directory-1/file-in-directory-1.txt s-link-to-file-1
        COMMAND ${CMAKE_COMMAND} -E chdir ${DATA_TEST_DIR} ln -s directory-2 s-link-to-dir-1
        COMMAND ${DEPS_BIN_DIR}/mkdwarfs -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm ${DATA_TEST_DIR}/*-link-to-* ${DATA_TEST_DIR}/directory-1/*empty.*
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    endif(${USE_TEMP_FS})
  else(${WITH_LINK_TESTS})
    if (${USE_TEMP_FS})
      add_custom_target(PACKAGED_FILESYSTEM ALL
        COMMAND ${DEPS_BIN_DIR}/mkdwarfs -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${DATA_TEST_DIR}
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    else(${USE_TEMP_FS})
      add_custom_target(PACKAGED_FILESYSTEM ALL
        COMMAND ${DEPS_BIN_DIR}/mkdwarfs -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm ${DATA_TEST_DIR}/directory-1/*empty.*
        BYPRODUCTS ${DATA_BIN_FILE}
      )
    endif(${USE_TEMP_FS})
  endif(${WITH_LINK_TESTS})

  add_dependencies(PACKAGED_FILESYSTEM ${DWARFS_PRJ} PREPACKAGED_FILESYSTEM)

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

  find_library(_LIBFMT "libfmt.a" REQUIRED)
  find_library(_LIBDC "libdouble-conversion.a" REQUIRED)
  find_library(_LIBEVENT "libevent.a" REQUIRED)
  find_library(_LIBCRYPTO "libcrypto.a" REQUIRED)
  find_library(_LIBLZ4 "liblz4.a" REQUIRED)
  find_library(_LIBLZMA "liblzma.a" REQUIRED)
  find_library(_LIBZ "libz.a" REQUIRED)
  find_library(_LIBLJEMALLOC "libjemalloc.a" REQUIRED)
  find_library(_LIBSSL "libssl.a" REQUIRED)

  find_library(_LIBACL "libacl.a")
  find_library(_LIBPTHREAD "libpthread.a")
  find_library(_LIBDL "libdl.a")
  find_library(_LIBC "libc.a")
  find_library(_LIBM "libm.a")
  find_library(_LIBRT "librt.a")
  find_library(_LIBUNWIND "libunwind.a")
  find_library(_LIBIBERTY "libiberty.a")

 # ...................................................................
 # The very special test
 # We check that (1) it is possible to compile, build and run program with pure C startup
 #               (2) it is statically linked

  if (NOT CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    add_link_options(-static)
  endif()


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
    "tests/tests-dir-io.cpp"
    "tests/tests-ln.cpp"
    "tests/tests-dl.cpp"
    "tests/tebako-fs.cpp"
    "tests/tebako-fs.h"
    "tests/tests.h"
  )

# ...................................................................
#  IMPORTANT !!!
#  MUST be in this order otherwise link may fail

  target_link_libraries(wr-bin dwarfs-wr)

  target_compile_options(wr-tests PUBLIC ${GTEST_CFLAGS})
  target_link_libraries(wr-tests dwarfs-wr ${GTEST_LDFLAGS})
  gtest_add_tests(TARGET wr-tests)

  add_dependencies(wr-bin ${INCBIN_PRJ} PACKAGED_FILESYSTEM)
  add_dependencies(wr-tests ${INCBIN_PRJ} PACKAGED_FILESYSTEM wr-bin)

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
  add_dwarfs_library(_LIBARCHIVE ${LIBARCHIVE_PRJ} wr-bin)

  target_link_libraries(wr-tests _LIBDWARFS  _LIBFSST   _LIBFOLLY _LIBT_METADATA
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

  list(APPEND _LIBRARIES ${_LIBFMT} ${Boost_LIBRARIES} ${_LIBDC} ${_LIBEVENT})

  if (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    list(APPEND _LIBRARIES ${_LIBLZ4} ${_LIBZ} ${_LIBSSL} ${_LIBCRYPTO} ${_LIBJEMALLOC} ${_LIBLZMA})
  else()
    list(APPEND _LIBRARIES ${_LIBIBERTY} ${_LIBACL}      ${_LIBSSL}      ${_LIBCRYPTO}  ${_LIBLZ4}
                           ${_LIBZ}      ${_LIBJEMALLOC} ${_LIBPTHREAD}  ${_LIBDL}      ${_LIBM}
                           ${_LIBRT}     gcc_eh          ${_LIBUNWIND}   ${_LIBLZMA}
    )
  endif()

  target_link_libraries(wr-bin ${_LIBRARIES})
  target_link_libraries(wr-tests gtest_main gtest ${_LIBRARIES})

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

install(FILES
        "include/tebako-defines.h"
        "include/tebako-io.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tebako
)

# We do not attempt to install fuse driver since we may operate in the environment
# where fuse is not vailable at all
install(FILES
        ${DEPS_BIN_DIR}/dwarfsck
        ${DEPS_BIN_DIR}/dwarfsextract
        ${DEPS_BIN_DIR}/mkdwarfs
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)
