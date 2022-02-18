# Copyright (c) 2021-2022, [Ribose Inc](https://www.ribose.com).
# All rights reserved.
# This file is a part of tamatebako
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
#

set(WITH_JEMALLOC_BUILD OFF)

if (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
# If we are cross compiling TARGET_HOMEBREW will point to homebrew environment for target
# If we are not cross compiling it will be empty

  if(NOT TARGET_HOMEBREW)
    set(BREW_BIN brew)
  else()
    set(BREW_BIN "${TARGET_HOMEBREW}/bin/brew" )
  endif()

  execute_process(
    COMMAND "${BREW_BIN}" --prefix
      RESULT_VARIABLE BREW_PREFIX_RES
      OUTPUT_VARIABLE TARGET_BREW_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT (BREW_PREFIX_RES EQUAL 0 AND EXISTS ${TARGET_BREW_PREFIX}))
    message(FATAL "Could not find target brew setup")
  endif()

  message(STATUS "Using target brew environment at ${TARGET_BREW_PREFIX}")
  set(CMAKE_PREFIX_PATH "${TARGET_BREW_PREFIX};${TARGET_BREW_PREFIX}/opt/openssl@1.1;${TARGET_BREW_PREFIX}/opt/zlib")
  include_directories("${TARGET_BREW_PREFIX}/include")

# Suppress superfluous randlib warnings about "*.a" having no symbols on MacOSX.
  set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
  set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")

  set(CMAKE_CXX_FLAGS "-DTARGET_OS_SIMULATOR=0 -DTARGET_OS_IPHONE=0")

  set(WITH_JEMALLOC_BUILD ON)

endif()
