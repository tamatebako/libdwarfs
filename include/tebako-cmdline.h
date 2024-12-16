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

#pragma once

#include "tebako-package-descriptor.h"

namespace tebako {

class cmdline_args {
  int argc;
  const char** argv;

  std::vector<std::string> mountpoints;
  std::vector<std::string> other_args;

  std::string extract_folder;
  std::string app_image;

  std::optional<package_descriptor> descriptor;
  std::vector<char> package;

  int new_argc;
  char** new_argv;
  char* new_argv_memory;

  bool extract;
  bool run;

 public:
  cmdline_args(int argc, const char** argv)
      : argc(argc), argv(argv), extract(false), run(false), new_argc(0), new_argv(nullptr), new_argv_memory(nullptr)
  {
  }
  ~cmdline_args()
  {
    if (new_argv_memory) {
      delete[] new_argv_memory;
    }
    if (new_argv) {
      delete[] new_argv;
    }
  }

  void parse_arguments(void);
  void process_mountpoints();
  void process_package();

  void build_arguments(const char* fs_mount_point, const char* fs_entry_point);
  void build_arguments_for_extract(const char* fs_mount_point);
  void build_arguments_for_run(const char* fs_mount_point, const char* fs_entry_point);

  std::vector<std::string> const& get_mountpoints() { return mountpoints; }
  std::vector<std::string> const& get_args() { return other_args; }

  int get_argc() { return new_argc; }
  char** get_argv() { return new_argv; }

  bool with_application() { return run; }
  bool shall_extract() { return extract; }
  std::string const& get_application_image() { return app_image; }

  std::optional<package_descriptor> const& get_descriptor() { return descriptor; }
  std::vector<char> const& get_package() { return package; }
};

}  // namespace tebako
