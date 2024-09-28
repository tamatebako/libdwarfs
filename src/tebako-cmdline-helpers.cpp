/**
 *
 * Copyright (c) 2024 [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of tebako
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include <string>
#include <cstdint>
#include <vector>

#include <tebako-cmdline-helpers.h>

namespace tebako {

// tebako_extract_cmdline
//  Builds command line arguments to process --tebako-extract
//  ruby -e "require 'fileutils'; FileUtils.copy_entry '<tebako::fs_mount_point>',argv[2] || 'source_filesystem'"
int tebako_extract_cmdline(int* argc, char*** argv, const char* fs_mount_point)
{
  int ret = -1;
  std::string dest = std::string(((*argc) < 3 ? "source_filesystem" : (*argv)[2]));
  std::string cmd = std::string("require 'fileutils'; FileUtils.copy_entry '") + fs_mount_point + "', '" + dest + "'";
  printf("Extracting tebako image to '%s' \n", dest.c_str());
  size_t new_argv_size = 3 + cmd.size() + 1 + strlen((*argv)[0]) + 1;
  char** new_argv = new char*[3];
  char* argv_memory = new char[new_argv_size];
  if (new_argv != NULL && argv_memory != NULL) {
    strcpy(argv_memory, (*argv)[0]);
    new_argv[0] = argv_memory;
    argv_memory += (strlen((*argv)[0]) + 1);
    strcpy(argv_memory, "-e");
    new_argv[1] = argv_memory;
    argv_memory += 3;
    strcpy(argv_memory, cmd.c_str());
    new_argv[2] = argv_memory;
    *argv = new_argv;
    (*argc) = 3;
    ret = 0;
  }
  return ret;
}
}  // namespace tebako
