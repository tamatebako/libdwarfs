/**
 *
 * Copyright (c) 2024, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of tebako (libdwarfs-wr)
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
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "tests.h"
#include <tebako-cmdline-helpers.h>

namespace tebako {

// Test case 1: Basic case with no additional arguments
TEST(ExtractCmdlineTest, no_additional_args)
{
  int argc = 1;

  char** p;
  char** argv = p = new char*[2];
  char* pp[1];

  argv[0] = pp[0] = strdup("program_name");
  argv[1] = nullptr;

  const char* fs_mount_point = "/mnt/point";

  int result = build_arguments_for_extract(&argc, &argv, fs_mount_point);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(argc, 3);
  EXPECT_STREQ(argv[0], "program_name");
  EXPECT_STREQ(argv[1], "-e");
  EXPECT_STREQ(argv[2], "require 'fileutils'; FileUtils.copy_entry '/mnt/point', 'source_filesystem'");

  free(pp[0]);
  delete[] p;

  delete[] argv[0];
  delete[] argv;
}

// Test case 2: With a custom destination argument
TEST(ExtractCmdlineTest, custom_destination_arg)
{
  int argc = 3;

  char** p;
  char** argv = p = new char*[4];
  char* pp[3];

  argv[0] = pp[0] = strdup("program_name");
  argv[1] = pp[1] = strdup("--tebako-extract");
  argv[2] = pp[2] = strdup("custom_dest");
  argv[3] = nullptr;

  const char* fs_mount_point = "/mnt/point";

  int result = build_arguments_for_extract(&argc, &argv, fs_mount_point);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(argc, 3);
  EXPECT_STREQ(argv[0], "program_name");
  EXPECT_STREQ(argv[1], "-e");
  EXPECT_STREQ(argv[2], "require 'fileutils'; FileUtils.copy_entry '/mnt/point', 'custom_dest'");

  free(pp[0]);
  free(pp[1]);
  free(pp[2]);
  delete[] p;

  delete[] argv[0];
  delete[] argv;
}

// Test case 3: Long fs_mount_point input
TEST(ExtractCmdlineTest, long_fs_mount_point)
{
  int argc = 1;

  char** p;
  char** argv = p = new char*[2];
  char* pp[1];

  argv[0] = pp[0] = strdup("program_name");
  argv[1] = nullptr;

  std::string long_fs_mount_point(5000, 'a');  // Very long string

  int result = build_arguments_for_extract(&argc, &argv, long_fs_mount_point.c_str());

  EXPECT_EQ(result, 0);
  EXPECT_EQ(argc, 3);
  EXPECT_STREQ(argv[0], "program_name");
  EXPECT_STREQ(argv[1], "-e");
  std::string expected_cmd =
      "require 'fileutils'; FileUtils.copy_entry '" + long_fs_mount_point + "', 'source_filesystem'";
  EXPECT_STREQ(argv[2], expected_cmd.c_str());

  free(pp[0]);
  delete[] p;

  delete[] argv[0];
  delete[] argv;
}

TEST(ProcessArgumentsTest, handles_mount_rule)
{
  const char* argv[] = {"program", "--tebako-mount", "d1:m1", "--tebako-mount", "d2:m2", "other"};
  int argc = 6;

  auto [tebako_mount_args, other_args] = parse_arguments(argc, const_cast<char**>(argv));

  // Check that we correctly extracted the tebako-mount rules
  EXPECT_EQ(tebako_mount_args.size(), 2);
  EXPECT_EQ(tebako_mount_args[0], "d1:m1");
  EXPECT_EQ(tebako_mount_args[1], "d2:m2");

  // Check that we correctly placed other arguments
  EXPECT_EQ(other_args.size(), 2);
  EXPECT_EQ(other_args[0], "program");
  EXPECT_EQ(other_args[1], "other");
}

TEST(ProcessArgumentsTest, throws_error_on_missing_rule)
{
  const char* argv[] = {"program", "--tebako-mount"};
  int argc = 2;

  // Expect an exception due to missing rule
  EXPECT_THROW(parse_arguments(argc, const_cast<char**>(argv)), std::invalid_argument);
}

TEST(ProcessArgumentsTest, throws_error_on_option_in_place_of_rule)
{
  const char* argv[] = {"program", "--tebako-mount", "--option"};
  int argc = 3;

  // Expect an exception due to missing rule after "="
  EXPECT_THROW(parse_arguments(argc, const_cast<char**>(argv)), std::invalid_argument);
}

TEST(ProcessArgumentsTest, throws_error_on_on_no_rule)
{
  const char* argv[] = {"program", "--tebako-mount"};
  int argc = 2;

  // Expect an exception due to missing rule after "="
  EXPECT_THROW(parse_arguments(argc, const_cast<char**>(argv)), std::invalid_argument);
}

TEST(ProcessArgumentsTest, handles_no_tebako_mount_argument)
{
  const char* argv[] = {"program", "arg1", "arg2"};
  int argc = 3;

  auto [tebako_mount_args, other_args] = parse_arguments(argc, const_cast<char**>(argv));

  // No tebako-mount arguments should be found
  EXPECT_TRUE(tebako_mount_args.empty());

  // Check that all arguments were placed in other_args
  EXPECT_EQ(other_args.size(), 3);
  EXPECT_EQ(other_args[0], "program");
  EXPECT_EQ(other_args[1], "arg1");
  EXPECT_EQ(other_args[2], "arg2");
}

// Helper function to clean up argv memory
static void clean_up_argv(int argc, char** argv)
{
  if (argv) {
    delete[] argv[0];  // This deletes the memory block used for all strings
    delete[] argv;     // This deletes the array of char pointers
  }
}

// Test Case 1: Basic Case
TEST(BuildArgumentsTest, basic_case)
{
  std::vector<std::string> new_argv = {"program", "arg1", "arg2"};
  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "/local/entry";

  auto [argc, argv] = build_arguments(new_argv, fs_mount_point, fs_entry_point);

  ASSERT_EQ(argc, 4);
  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "/mnt/local/entry");
  EXPECT_STREQ(argv[2], "arg1");
  EXPECT_STREQ(argv[3], "arg2");

  clean_up_argv(argc, argv);
}

// Test Case 2: No Arguments
TEST(BuildArgumentsTest, no_arguments)
{
  std::vector<std::string> new_argv = {"program"};
  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "/local/entry";

  auto [argc, argv] = build_arguments(new_argv, fs_mount_point, fs_entry_point);

  ASSERT_EQ(argc, 2);
  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "/mnt/local/entry");

  clean_up_argv(argc, argv);
}

// Test Case 3: Long Strings
TEST(BuildArgumentsTest, long_strings)
{
  std::vector<std::string> new_argv = {"program", std::string(1000, 'a'), std::string(2000, 'b')};
  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "/local/entry";

  auto [argc, argv] = build_arguments(new_argv, fs_mount_point, fs_entry_point);

  ASSERT_EQ(argc, 4);
  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "/mnt/local/entry");
  EXPECT_EQ(std::string(argv[2]), std::string(1000, 'a'));
  EXPECT_EQ(std::string(argv[3]), std::string(2000, 'b'));

  clean_up_argv(argc, argv);
}

// Test Case 4: Special Characters
TEST(BuildArgumentsTest, special_characters)
{
  std::vector<std::string> new_argv = {"program", "arg 1", "arg\t2", "arg@3"};
  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "/local/entry";

  auto [argc, argv] = build_arguments(new_argv, fs_mount_point, fs_entry_point);

  ASSERT_EQ(argc, 5);
  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "/mnt/local/entry");
  EXPECT_STREQ(argv[2], "arg 1");
  EXPECT_STREQ(argv[3], "arg\t2");
  EXPECT_STREQ(argv[4], "arg@3");

  clean_up_argv(argc, argv);
}

// Test Case 5: Empty Mount Point and Entry Point
TEST(BuildArgumentsTest, empty_mount_or_entry_points)
{
  std::vector<std::string> new_argv = {"program", "arg1"};
  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "entry";

  EXPECT_THROW(build_arguments(new_argv, "", fs_entry_point), std::invalid_argument);
  ;
  EXPECT_THROW(build_arguments(new_argv, fs_mount_point, ""), std::invalid_argument);
  ;
}

// Test Case 6: Null Mount or Entry Point
TEST(BuildArgumentsTest, null_mount_or_entry_points)
{
  std::vector<std::string> new_argv = {"program", "arg1"};

  EXPECT_THROW(build_arguments(new_argv, nullptr, "/entry"), std::invalid_argument);
  EXPECT_THROW(build_arguments(new_argv, "/mnt", nullptr), std::invalid_argument);
}

}  // namespace tebako