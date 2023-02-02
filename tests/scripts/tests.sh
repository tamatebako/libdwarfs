#! /bin/bash
#
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

# More safety, by turning some bugs into errors.
# Without `errexit` you don’t need ! and can replace
# PIPESTATUS with a simple $?
set -o errexit -o pipefail -o noclobber -o nounset

# Checks referenced shared libraries
# $1 - an array of expected refs to shared libraries
# $2 - an array of actual refs to shared libraries
check_shared_libs() {
   expected_size="${#expected[@]}"
   actual_size="${#actual[@]}"
   assertEquals "The number of references to shared libraries does not meet our expectations" "$expected_size" "$actual_size"

   for exp in "${expected[@]}"; do
      for i in "${!actual[@]}"; do
         if [[ "$OSTYPE" == "msys" ]]; then
            actual_i=${actual[i],,}
         else
            actual_i=${actual[i]}
         fi
         if [[ "$actual_i" == *"$exp"* ]]; then
           unset 'actual[i]'
         fi
      done
   done

   for unexp in "${actual[@]}"; do
      echo "Unxpected reference to shared library $unexp"
   done

   assertEquals "Unxpected references to shared libraries" 0 "${#actual[@]}"
}

# ......................................................................
# Run ldd to check that wr-bin has been linked to expected set of shared libraries
test_linkage() {
   echo "==> References to shared libraries test"
   if [[ "$ASAN" == "ON"* || "$COVERAGE" == "ON"* ]]; then
      echo "... Address sanitizer or coverage test is on ... skipping"
   else
      if [[ "$OSTYPE" == "linux-gnu"* ]]; then
         expected=("linux-vdso.so" "libpthread.so" "libdl.so" "libm.so" "libgcc_s.so" "libc.so" "ld-linux-x86-64.so")
         readarray -t actual < <(ldd "$DIR_SRC/wr-bin")
         assertEquals "readarray -t actual < <(ldd $DIR_SRC/wr-bin) failed" 0 "${PIPESTATUS[0]}"
         check_shared_libs
# Used to be:
# Run ldd to check that wr-bin has been linked statically
#        result="$( ldd "$DIR_ROOT"/wr-bin 2>&1 )"
#        assertEquals 1 "${PIPESTATUS[0]}"
#        assertContains "$result" "not a dynamic executable"
      elif [[ "$OSTYPE" == "linux-musl"* ]]; then
         expected=("libgcc_s.so" "libc.musl-x86_64.so" "ld-musl-x86_64.so")
         readarray -t actual < <(ldd "$DIR_SRC/wr-bin")
         assertEquals "readarray -t actual < <(ldd $DIR_SRC/wr-bin) failed" 0 "${PIPESTATUS[0]}"
         check_shared_libs
      elif [[ "$OSTYPE" == "darwin"* ]]; then
         expected=("libc++.1.dylib" "libSystem.B.dylib" "wr-bin")
         readarray -t actual < <(otool -L "$DIR_SRC/wr-bin")
         assertEquals "readarray -t actual < <(otool -L $DIR_SRC/wr-bin) failed" 0 "${PIPESTATUS[0]}"
         check_shared_libs "${expected[@]}"
      elif [[ "$OSTYPE" == "cygwin" ]]; then
         echo "... cygwin ... skipping"
      elif [[ "$OSTYPE" == "msys" ]]; then
         if [[ "$RB_W32" == "ON" ]]; then
            expected=("ntdll.dll" "kernel32.dll" "kernelbase.dll" "advapi32.dll" "msvcrt.dll"
                      "sechost.dll" "rpcrt4.dll" "shlwapi.dll" "user32.dll" "win32u.dll" "gdi32.dll"
                      "gdi32full.dll" "msvcp_win.dll" "ucrtbase.dll" "ws2_32.dll" "wsock32.dll"
                      "imagehlp.dll" "shell32.dll" "iphlpapi.dll")
         else
            expected=("ntdll.dll" "kernel32.dll" "kernelbase.dll" "advapi32.dll" "msvcrt.dll"
                      "sechost.dll" "rpcrt4.dll" "shlwapi.dll" "user32.dll" "win32u.dll" "gdi32.dll"
                      "gdi32full.dll" "msvcp_win.dll" "ucrtbase.dll" "ws2_32.dll" "wsock32.dll")
         fi
         readarray -t actual < <(ldd "$DIR_SRC/wr-bin.exe")
         assertEquals "readarray -t actual < <(ldd $DIR_SRC/wr-bin.exe) failed" 0 "${PIPESTATUS[0]}"
         check_shared_libs
      elif [[ "$OSTYPE" == "win32" ]]; then
         echo "... win32 ... skipping"
      elif [[ "$OSTYPE" == "freebsd"* ]]; then
         echo "... freebsd ... skipping"
      else
         echo "... unknown - $OSTYPE ... skipping"
      fi
   fi
}

# ......................................................................
# Check "C" interface bindings in statically linked program
# Check that tmp directory is cleaned upon shutdown
test_C_bindings_and_temp_dir() {
   echo "==> C bindings and temp dir handling combined test"

# Skip if cross-compiling
# (Based on the assumption that the only possible cross compile scenario is MacOs [x86_64 --> arm 64])
   [ "${TARGET:-}" == "arm64" ] && startSkipping

   mkdir "$DIR_TESTS"/temp
   assertEquals "$DIR_TESTS/temp failed" 0 "${PIPESTATUS[0]}"

   ls /tmp > "$DIR_TESTS"/temp/before
   assertEquals "ls /tmp > $DIR_TESTS/temp/before failed" 0 "${PIPESTATUS[0]}"

   if [[ "$OSTYPE" == "msys" ]]; then
      WR_BIN="$DIR_SRC"/wr-bin.exe
   else
      WR_BIN="$DIR_SRC"/wr-bin
   fi

   "$WR_BIN"

   assertEquals "$DIR_ROOT/wr-bin failed" 0 "${PIPESTATUS[0]}"

   ls /tmp > "$DIR_TESTS"/temp/after
   assertEquals "ls /tmp > $DIR_TESTS/temp/after failed" 0 "${PIPESTATUS[0]}"

   diff "$DIR_TESTS"/temp/before "$DIR_TESTS"/temp/after
   assertEquals "$DIR_TESTS/temp/before $DIR_TESTS/temp/after differ" 0 "${PIPESTATUS[0]}"

   rm -rf "$DIR_TESTS"/temp
   assertEquals "rm -rf $DIR_TESTS/temp  failed" 0 "${PIPESTATUS[0]}"
}

# ......................................................................
# Check that libdwarfs_wr, dwarfs utilities and all dependencies are installed as expected
test_install_script() {
   echo "==> Install script test"

# Skip if cross-compiling
# (Based on the assumption that the only possible cross compile scenario is MacOs [x86_64 --> arm 64])
   [ "${TARGET:-}" == "arm64" ] && startSkipping

   DIR_INSTALL="$DIR_ROOT"/install
   DIR_INS_B="$DIR_INSTALL"/bin
   DIR_INS_L="$DIR_INSTALL"/lib
   DIR_INS_I="$DIR_INSTALL"/include/tebako

   if [[ "$OSTYPE" == "msys" ]]; then
      NM_MKDWARFS="$DIR_INS_B/mkdwarfs.exe"
      NM_LIBARCHIVE="$DIR_INS_L/libarchive_static.a"
   else
      NM_MKDWARFS="$DIR_INS_B/mkdwarfs"
      NM_LIBARCHIVE="$DIR_INS_L/libarchive.a"
   fi

   cmake --install  "$DIR_SRC" --prefix "$DIR_INSTALL"
   assertEquals "cmake --install failed" 0 "${PIPESTATUS[0]}"

# We do not test fuse driver because we may operate in the environment
# where fuse is not vailable at all
#   assertTrue "[ -f "$DIR_INS_B"/dwarfs2 ]"

# We do not test dwarfsck and dwarfsextract since v0.1.9 because it employ optimized
# configuration and those two utilities are not built
#   assertTrue ""$DIR_INS_B"/dwarfsck was not installed" "[ -f "$DIR_INS_B"/dwarfsck ]"
#   assertTrue ""$DIR_INS_B"/dwarfsextract was not installed" "[ -f "$DIR_INS_B"/dwarfsextract ]"

   assertTrue "$NM_MKDWARFS was not installed" "[ -f $NM_MKDWARFS ]"
   assertTrue "$DIR_INS_L/libdwarfs-wr.a was not installed" "[ -f $DIR_INS_L/libdwarfs-wr.a ]"
   assertTrue "$DIR_INS_L/libdwarfs.a was not installed" "[ -f $DIR_INS_L/libdwarfs.a ]"
   assertTrue "$DIR_INS_L/libfsst.a was not installed" "[ -f $DIR_INS_L/libfsst.a ]"
   assertTrue "$DIR_INS_L/libfolly.a was not installed" "[ -f $DIR_INS_L/libfolly.a ]"
   assertTrue "$DIR_INS_L/libmetadata_thrift.a was not installed" "[ -f $DIR_INS_L/libmetadata_thrift.a ]"
   assertTrue "$DIR_INS_L/libthrift_light.a was not installed" "[ -f $DIR_INS_L/libthrift_light.a ]"
   assertTrue "$DIR_INS_L/libxxhash.a was not installed" "[ -f $DIR_INS_L/libxxhash.a ]"
   assertTrue "$DIR_INS_L/libzstd.a was not installed" "[ -f $DIR_INS_L/libzstd.a ]"
   assertTrue "$NM_LIBARCHIVE was not installed" "[ -f $NM_LIBARCHIVE ]"
   assertTrue "$DIR_INS_I/tebako-config.h was not installed" "[ -f $DIR_INS_I/tebako-config.h ]"
   assertTrue "$DIR_INS_I/tebako-defines.h was not installed" "[ -f $DIR_INS_I/tebako-defines.h ]"
   assertTrue "$DIR_INS_I/tebako-io.h was not installed" "[ -f $DIR_INS_I/tebako-io.h ]"
   assertTrue "$DIR_INS_I/tebako-io-rb-w32.h was not installed" "[ -f $DIR_INS_I/tebako-io-rb-w32.h ]"
}

# ......................................................................
# main
DIR0="$( cd "$( dirname "$0" )" && pwd )"
DIR1="${DIR_ROOT:="$DIR0/../.."}"
DIR_ROOT="$( cd "$DIR1" && pwd )"
DIR_SRC="$DIR_ROOT"/build
DIR_TESTS="$( cd "$DIR0/.." && pwd)"

ASAN="${ASAN:=OFF}"
COVERAGE="${COVERAGE:=OFF}"
RB_W32="${RB_W32:=OFF}"

echo "Running libdwarfs additional tests"
# shellcheck source=/dev/null
. "$DIR_TESTS"/shunit2/shunit2
