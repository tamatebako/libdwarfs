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

project(libdwarfs-wr VERSION 0.1.0)

include(ExternalProject)
include(FetchContent)
include(GoogleTest)
include(CTest)
include(${CMAKE_SOURCE_DIR}/cmake/def_ext_prj.cmake)

enable_testing()

option(SYSTEM_GTESTS "prefer system gtest" ON)

set(Boost_USE_STATIC_LIBS   ON)
set(BOOST_LINK_STATIC       ON)
set(USE_STATIC_DEPS_ON_UNIX ON)
set(GFLAGS_SHARED          OFF)


# Various locations for external projects 
set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps CACHE STRING "Dependencies' folder'")
set(DEPS_INCLUDE_DIR ${DEPS}/include)
set(DEPS_LIB_DIR ${DEPS}/lib)
set(DEPS_BIN_DIR ${DEPS}/bin)
set(DEPS_SBIN_DIR ${DEPS}/sbin)

# ...................................................................
# External projects

def_ext_prj_g(DWARFS "78401c3")
def_ext_prj_g(INCBIN "348e36b")

message("Collecting packages:")
message("incbin   - " @${INCBIN_TAG}         " at " ${INCBIN_SOURCE_DIR}) 
message("dwarfs   - " @${DWARFS_TAG}         " at " ${DWARFS_SOURCE_DIR}) 

# ...................................................................
# incbin
# ExternalProject_Add is used for incbin project since we do not need 
# anything at configure step. We want to fetch some files before the 
# build, that's it.  

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
set(__LIBARCHIVE "${DWARFS_BINARY_DIR}/libarchive/lib/libarchive.a")

ExternalProject_Add(${DWARFS_PRJ}
    PREFIX ${DEPS}
    GIT_REPOSITORY https://github.com/tamatebako/dwarfs.git
    GIT_TAG ${DWARFS_TAG}
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/patches/2.patch/version.cpp ${DEPS}/src/_dwarfs/src/dwarfs  
    COMMAND       ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/patches/2.patch/version.h ${DEPS}/src/_dwarfs/include/dwarfs   
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEPS} -DSTATIC_BUILD_DO_NOT_USE:BOOL=ON
    SOURCE_DIR ${DWARFS_SOURCE_DIR}
    BINARY_DIR ${DWARFS_BINARY_DIR}
    BUILD_BYPRODUCTS ${__LIBDWARFS} ${__LIBFSST} ${__LIBFOLLY} ${__LIBT_METADATA} 
                     ${__LIBT_LIGHT} ${__LIBXXHASH} ${__LIBZSTD} ${__LIBARCHIVE}
)

# ...................................................................
# Library

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/resources/version.h.in 
   ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h
   @ONLY
)

add_compile_options(-fdiagnostics-color=always)
#[TODO]
#add_link_options(-static -static-libgcc)
add_link_options(-static)
set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2 -g")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${INCBIN_SOURCE_DIR}
                    ${DWARFS_SOURCE_DIR}/folly ${DWARFS_BINARY_DIR}/folly ${DWARFS_SOURCE_DIR}/fbthrift 
                    ${DWARFS_BINARY_DIR}/thrift ${DWARFS_BINARY_DIR} ${DWARFS_SOURCE_DIR}/include
)

add_library(dwarfs-wr STATIC 
    "src/getcwd.c"
    "src/chdir.c"
    "src/stat.c"
    "src/access.c"
    "src/mkdir.c"
    "src/open.c"
    "src/tebako-io-helpers.cpp"
    "src/tebako-mfs.cpp"
    "src/tebako-dfs.cpp"
    "src/tebako-fd.cpp"
    "include/tebako-pch.h"
    "include/tebako-common.h"
    "include/tebako-io.h"
    "include/tebako-io-inner.h"
    "include/tebako-mfs.h"
    "include/tebako-defines.h" 
    "include/version.h"
     )

set_property(TARGET dwarfs-wr PROPERTY CXX_STANDARD 17)
set_property(TARGET dwarfs-wr PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET dwarfs-wr PROPERTY CXX_EXTENSIONS OFF)

add_dependencies(dwarfs-wr ${DWARFS_PRJ})

# ...................................................................
# Tests

find_package(PkgConfig REQUIRED)
pkg_search_module(GTEST gtest_main)

#FetchContent_Declare(
#   googletest
# Specify the commit you depend on and update it regularly.
#   URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
#)
# For Windows: Prevent overriding the parent project's compiler/linker settings
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#FetchContent_MakeAvailable(googletest)


# ...................................................................
# Filesystem locations

# DATA_TEST_DIR folder contains raw data for filesystem tests 
  set(DATA_TEST_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_filesystem)
# DATA_BIN_DIR folder is used to create packaged filesystem  
  set(DATA_BIN_DIR  ${CMAKE_CURRENT_BINARY_DIR})
# DATA_BIN_FILE is packaged filesystem itself
  set(DATA_BIN_FILE ${DATA_BIN_DIR}/fs.bin)

# ...................................................................
# Packaged filesystem

add_custom_target(PACKAGED_FILESYSTEM ALL
   COMMAND ${DEPS_BIN_DIR}/mkdwarfs -o ${DATA_BIN_FILE} -i ${DATA_TEST_DIR}
   DEPENDS ${DWARFS_PRJ} 
   BYPRODUCTS ${DATA_BIN_FILE}
)

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

 find_package(Boost 1.67 REQUIRED COMPONENTS context regex thread atomic date_time filesystem)

 find_library(_LIBGLOG "libglog.a" REQUIRED)
 find_library(_LIBFMT "libfmt.a" REQUIRED)
 find_library(_LIBDC "libdouble-conversion.a" REQUIRED)
 find_library(_LIBGFLAGS "libgflags.a" REQUIRED)
 find_library(_LIBEVENT "libevent.a" REQUIRED)
 find_library(_LIBACL "libacl.a" REQUIRED) 
 find_library(_LIBCRYPTO "libcrypto.a" REQUIRED) 
 find_library(_LIBLZ4 "liblz4.a" REQUIRED) 
 find_library(_LIBLZMA "liblzma.a" REQUIRED) 
 find_library(_LIBZ "libz.a" REQUIRED) 
 find_library(_LIBLJEMALLOC "libjemalloc.a" REQUIRED) 
 find_library(_LIBPTHREAD "libpthread.a" REQUIRED) 
 find_library(_LIBDL "libdl.a" REQUIRED) 
 find_library(_LIBC "libc.a" REQUIRED)  
 find_library(_LIBM "libm.a" REQUIRED)
 find_library(_LIBRT "librt.a" REQUIRED)
 find_library(_LIBSSL "libssl.a" REQUIRED)
 find_library(_LIBUNWIND "libunwind.a" REQUIRED)
 find_library(_LIBIBERTY "libiberty.a" REQUIRED)


 # ...................................................................
 # The very special test
 # We check that (1) it is possible to compile, build and run program with pure C startup
 #               (2) it is statically linked

add_executable(wr-bin
  "tests/main.c"
  "tests/tebako-fs.cpp"
  "tests/tebako-fs.h"
  "tests/tests.h"
)
 
target_link_libraries(wr-bin  dwarfs-wr)
add_dependencies(wr-bin ${INCBIN_PRJ} PACKAGED_FILESYSTEM) 

# ...................................................................
# The libraries that are build by dwarfs project
# libdwarfs libfolly libfsst libxxhash libmetadata_thrift libthrift_light libarchive

add_dwarfs_library(_LIBDWARFS ${DWARFS_PRJ} wr-bin)
add_dwarfs_library(_LIBFSST ${DWARFS_PRJ} wr-bin)
add_dwarfs_library(_LIBFOLLY ${DWARFS_PRJ} wr-bin)
add_dwarfs_library(_LIBT_METADATA ${DWARFS_PRJ} wr-bin)
add_dwarfs_library(_LIBT_LIGHT ${DWARFS_PRJ} wr-bin)
add_dwarfs_library(_LIBXXHASH ${DWARFS_PRJ} wr-bin)
add_dwarfs_library(_LIBZSTD ${DWARFS_PRJ} wr-bin)
add_dwarfs_library(_LIBARCHIVE ${DWARFS_PRJ} wr-bin)

# ...................................................................
#  IMPORTANT !!!
#  MUST be in this order otherwise link may fail
#  -static-libgcc and gcc_eh below is all together an ugly trick to enforce static linking

 target_link_libraries(wr-bin                              
   ${_LIBFMT}          ${Boost_LIBRARIES}  ${_LIBDC}                               
   ${_LIBGLOG}         ${_LIBGFLAGS}       ${_LIBEVENT}        ${_LIBIBERTY}
   ${_LIBACL}          ${_LIBSSL}          ${_LIBCRYPTO}       ${_LIBLZ4}
   ${_LIBZ}            ${_LIBJEMALLOC}     ${_LIBPTHREAD}      ${_LIBDL}           
   ${_LIBM}            ${_LIBRT}           gcc_eh		                 
   ${_LIBUNWIND}       ${_LIBLZMA}
 )


# ...................................................................
# Google test

add_executable(wr-tests 
  "tests/load.cpp"
  "tests/stat.cpp"
  "tests/access.cpp"
  "tests/dir.cpp"
  "tests/file-io.cpp"
  "tests/tebako-fs.cpp"
  "tests/tebako-fs.h"
  "tests/tests.h"
 )

target_compile_options(wr-tests PUBLIC ${GTEST_CFLAGS})
target_link_libraries(wr-tests 
    dwarfs-wr
    ${GTEST_LDFLAGS}
)

add_dependencies(wr-tests ${INCBIN_PRJ} PACKAGED_FILESYSTEM) 

gtest_add_tests(TARGET wr-tests)

target_link_libraries(wr-tests _LIBDWARFS)
target_link_libraries(wr-tests _LIBFSST)
target_link_libraries(wr-tests _LIBFOLLY)
target_link_libraries(wr-tests _LIBT_METADATA)
target_link_libraries(wr-tests _LIBT_LIGHT)
target_link_libraries(wr-tests _LIBXXHASH)
target_link_libraries(wr-tests _LIBZSTD)
target_link_libraries(wr-tests _LIBARCHIVE)

# ...................................................................
#  IMPORTANT !!!
#  MUST be in this order otherwise link may fail
#  -static-libgcc and gcc_eh below is all together an ugly trick to enforce static linking

 target_link_libraries(wr-tests                              
   ${_LIBFMT}          ${Boost_LIBRARIES}  ${_LIBDC}                               
   ${_LIBGLOG}         ${_LIBGFLAGS}       ${_LIBEVENT}        ${_LIBIBERTY}
   ${_LIBACL}          ${_LIBSSL}          ${_LIBCRYPTO}       ${_LIBLZ4}
   ${_LIBZ}            ${_LIBJEMALLOC}     ${_LIBPTHREAD}      ${_LIBDL}           
   ${_LIBM}            ${_LIBRT}           gcc_eh		                 
   ${_LIBUNWIND}       ${_LIBLZMA}
 )

# ${_LIBC}

