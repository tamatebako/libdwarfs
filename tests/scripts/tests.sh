#! /bin/bash
#
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

# More safety, by turning some bugs into errors.
# Without `errexit` you don’t need ! and can replace
# PIPESTATUS with a simple $?
set -o errexit -o pipefail -o noclobber -o nounset

# ......................................................................
# Run ldd to check that wr-bin has been linked statically
test_static_linkage() {
   echo "==> Static linkage test"
   result="$( ldd "$DIR_ROOT"/wr-bin 2>&1 )"
   assertEquals 1 "${PIPESTATUS[0]}"
   assertContains "$result" "not a dynamic executable"
}

# ......................................................................
# Check "C" interface bindings in statically linked program
# Check that tmp directory is cleaned upon shutdown
test_C_bindings_and_temp_dir() {
   echo "==> C bindings and temp dir handling combined test"
   mkdir "$DIR_TESTS"/temp
   assertEquals 0 "${PIPESTATUS[0]}"

   ls /tmp > "$DIR_TESTS"/temp/before
   assertEquals 0 "${PIPESTATUS[0]}"

   "$DIR_ROOT"/wr-bin
   assertEquals 0 "${PIPESTATUS[0]}"

   ls /tmp > "$DIR_TESTS"/temp/after
   assertEquals 0 "${PIPESTATUS[0]}"

   diff "$DIR_TESTS"/temp/before "$DIR_TESTS"/temp/after
   assertEquals 0 "${PIPESTATUS[0]}"
}

# ......................................................................
# Check that libdwarfs_wr, dwarfs utilities and all dependencies are installed as expected
test_install_script() {
   echo "==> Install script test"

   DIR_INSTALL="$DIR_ROOT"/install
   DIR_INS_B="$DIR_INSTALL"/bin
   DIR_INS_L="$DIR_INSTALL"/lib
   DIR_INS_I="$DIR_INSTALL"/include/tebako

   cmake --install  "$DIR_ROOT" --prefix "$DIR_INSTALL"
   assertEquals 0 "${PIPESTATUS[0]}"

# We do not test fuse driver since we may operate in the environment 
# where fuse is not vailable at all
#   assertTrue "[ -f "$DIR_INS_B"/dwarfs2 ]"

   assertTrue "[ -f "$DIR_INS_B"/dwarfsck ]"
   assertTrue "[ -f "$DIR_INS_B"/dwarfsextract ]"
   assertTrue "[ -f "$DIR_INS_B"/mkdwarfs ]"
   assertTrue "[ -f "$DIR_INS_L"/libdwarfs-wr.a ]"
   assertTrue "[ -f "$DIR_INS_L"/libdwarfs.a ]"
   assertTrue "[ -f "$DIR_INS_L"/libfsst.a ]"
   assertTrue "[ -f "$DIR_INS_L"/libfolly.a ]"
   assertTrue "[ -f "$DIR_INS_L"/libmetadata_thrift.a ]"
   assertTrue "[ -f "$DIR_INS_L"/libthrift_light.a ]"
   assertTrue "[ -f "$DIR_INS_L"/libxxhash.a ]"
   assertTrue "[ -f "$DIR_INS_L"/libzstd.a ]"
   assertTrue "[ -f "$DIR_INS_L"/libarchive.a ]"
   assertTrue "[ -f "$DIR_INS_I"/tebako-defines.h ]"
   assertTrue "[ -f "$DIR_INS_I"/tebako-io.h ]"
}

# ......................................................................
# main
DIR0="$( cd "$( dirname "$0" )" && pwd )"
DIR_ROOT="$( cd "$DIR0"/../.. && pwd )"
DIR_TESTS="$( cd "$DIR_ROOT"/tests && pwd )"

echo "Running libdwarfs additional tests"
. "$DIR_TESTS"/shunit2/shunit2
