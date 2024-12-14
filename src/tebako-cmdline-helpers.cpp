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

#include <tebako-pch.h>
#include <tebako-pch-pp.h>
#include <tebako-common.h>
#include <tebako-io-inner.h>
#include <tebako-io-root.h>
#include <tebako-memfs.h>
#include <tebako-memfs-table.h>
#include <tebako-mount-table.h>

#include <tebako-cmdline-helpers.h>

namespace tebako {

// build_arguments_for_extract
//  Builds command line arguments to process --tebako-extract
//  ruby -e "require 'fileutils'; FileUtils.copy_entry '<tebako::fs_mount_point>',argv[2] || 'source_filesystem'"
int build_arguments_for_extract(int* argc, char*** argv, const char* fs_mount_point)
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

std::pair<int, char**> build_arguments(const std::vector<std::string>& new_argv,
                                       const char* fs_mount_point,
                                       const char* fs_entry_point)
{
  if (fs_mount_point == nullptr || fs_entry_point == nullptr || fs_mount_point[0] == 0 || fs_entry_point[0] == 0) {
    throw std::invalid_argument("Internal error: fs_mount_point and fs_entry_point must be non-null and non-empty");
  }

  size_t new_argv_size = strlen(fs_mount_point) + strlen(fs_entry_point) + 1;
  for (const auto& arg : new_argv) {
    new_argv_size += (arg.length() + 1);
  }

  char* argv_memory = new char[new_argv_size];
  if (!argv_memory)
    throw std::bad_alloc();

  // Build the new argv array
  char** new_argv_final = new char*[new_argv.size() + 1];
  memcpy(argv_memory, new_argv[0].c_str(), new_argv[0].length() + 1);
  new_argv_final[0] = argv_memory;
  argv_memory += (new_argv[0].length() + 1);

  // Add tebako fs_mount_point and fs_entry_point
  memcpy(argv_memory, fs_mount_point, strlen(fs_mount_point));
  new_argv_final[1] = argv_memory;
  argv_memory += strlen(fs_mount_point);

  memcpy(argv_memory, fs_entry_point, strlen(fs_entry_point) + 1);
  argv_memory += (strlen(fs_entry_point) + 1);

  // Copy remaining arguments
  for (size_t i = 1; i < new_argv.size(); i++) {
    memcpy(argv_memory, new_argv[i].c_str(), new_argv[i].length() + 1);
    new_argv_final[i + 1] = argv_memory;
    argv_memory += (new_argv[i].length() + 1);
  }

  // Return the new argc and argv
  return {static_cast<int>(new_argv.size() + 1), new_argv_final};
}

std::pair<std::vector<std::string>, std::vector<std::string>> parse_arguments(int argc, char** argv)
{
  const std::string error_msg =
      "Error: --tebako-mount must be followed by a rule (e.g., --tebako-mount <mount point>:<target>)";
  std::vector<std::string> tebako_mount_args;
  std::vector<std::string> other_args;

  for (int i = 0; i < argc; i++) {
    std::string arg = argv[i];

    // Handle "--tebako-mount=value" case
    if (arg.rfind("--tebako-mount=", 0) == 0) {  // Check if it starts with "--tebako-mount="
      std::string value = arg.substr(15);        // Extract the part after "--tebako-mount="

      if (!value.empty()) {
        tebako_mount_args.push_back(value);  // Add the value part to tebako_mount_args
        continue;
      }
      else {
        throw std::invalid_argument(error_msg);  // Handle case where value is empty
      }
    }

    // Handle "--tebako-mount" without '='
    if (arg == "--tebako-mount") {
      // Ensure there is a next argument
      if (i + 1 < argc) {
        std::string next_arg = argv[i + 1];

        // Check if the next argument is valid
        if (next_arg[0] != '-') {  // It's not a flag
          tebako_mount_args.push_back(next_arg);
          i += 1;  // Skip the next argument as it is the rule
          continue;
        }
        else {
          throw std::invalid_argument(error_msg);
        }
      }

      // If "--tebako-mount" is at the end of args without a rule, raise an error
      throw std::invalid_argument(error_msg);
    }

    // Add other arguments as they are
    other_args.push_back(arg);
  }

  return std::make_pair(tebako_mount_args, other_args);
}

void process_mountpoints(const std::vector<std::string>& mountpoints)
{
  for (const auto& item : mountpoints) {
    // Split item by the first ':' or '>'
    size_t separator_pos = item.find_first_of(":>");
    if (separator_pos != std::string::npos) {
      char separator = item[separator_pos];
      // Extract the first and second parts
      std::string mountpoint = item.substr(0, separator_pos);
      std::string target = item.substr(separator_pos + 1);

      // Split file_path into path and filename
      stdfs::path p(mountpoint);

      if (p.is_absolute()) {
        throw std::invalid_argument("Path " + mountpoint + " is not within tebako memfs");
      }

      std::string path = p.parent_path().string();
      std::string filename = p.filename().string();

      // Check that both filename and target are not empty
      if (filename.empty() || target.empty()) {
        throw std::invalid_argument("Invalid input: path or filename or terget is empty in " + item);
      }

      struct stat st;
      std::string lnk;
      uint32_t root = sync_tebako_memfs_table::get_tebako_memfs_table().get(0)->get_root_inode();
      int res = dwarfs_inode_relative_stat(root, path, &st, lnk, false);
      if (res == DWARFS_IO_ERROR) {
        throw std::invalid_argument("Path " + path + " does not exist or is not accessible");
      }
      if (res == DWARFS_S_LINK_OUTSIDE) {
        throw std::invalid_argument("Path " + path + " is not within tebako memfs");
      }

      if (separator == ':') {
        sync_tebako_mount_table::get_tebako_mount_table().insert(st.st_ino, filename, target);
      }
      else {  // assume that  (separator == '>')
        std::ifstream file(target, std::ios::binary | std::ios::ate);
        if (!file) {
          throw std::invalid_argument("Path " + target + " does not exist");
        }
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer;
        buffer.resize(size);

        if (!file.read(buffer.data(), size)) {
          throw std::invalid_argument("Failed to load filesystem image from " + target);
        }

        int ret = mount_memfs(buffer.data(), size, "auto", st.st_ino, filename.c_str());
        if (ret < 1) {
          throw std::invalid_argument("Failed to mount filesystem image from " + target);
        }
      }
    }
    else {
      throw std::invalid_argument("Invalid input: missing ':' or '>' separator in " + item);
    }
  }
}
}  // namespace tebako
