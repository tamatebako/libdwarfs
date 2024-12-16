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
#include <tebako-cmdline.h>

namespace tebako {

// Test case 1: Basic case with no additional arguments
TEST(CmdlineArgsTest, extract_no_additional_args)
{
  const int argc = 2;
  const char* argv[argc] = {"program_name", "--tebako-extract"};
  const char* fs_mount_point = "/mnt/point";

  cmdline_args args(argc, argv);
  args.parse_arguments();
  args.build_arguments_for_extract(fs_mount_point);

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 3);
  EXPECT_STREQ(new_argv[0], "program_name");
  EXPECT_STREQ(new_argv[1], "-e");
  EXPECT_STREQ(new_argv[2], "require 'fileutils'; FileUtils.copy_entry '/mnt/point', 'source_filesystem'");
}

TEST(CmdlineArgsTest, extract_no_additional_args_eq_case_1)
{
  const int argc = 2;
  const char* argv[argc] = {"program_name", "--tebako-extract="};
  const char* fs_mount_point = "/mnt/point";

  cmdline_args args(argc, argv);
  args.parse_arguments();
  args.build_arguments_for_extract(fs_mount_point);

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 3);
  EXPECT_STREQ(new_argv[0], "program_name");
  EXPECT_STREQ(new_argv[1], "-e");
  EXPECT_STREQ(new_argv[2], "require 'fileutils'; FileUtils.copy_entry '/mnt/point', 'source_filesystem'");
}

TEST(CmdlineArgsTest, extract_no_additional_args_eq_case_2)
{
  const int argc = 3;
  const char* argv[argc] = {"program_name", "--tebako-extract=", "--other-arg"};
  const char* fs_mount_point = "/mnt/point";

  cmdline_args args(argc, argv);
  args.parse_arguments();
  args.build_arguments_for_extract(fs_mount_point);

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 3);
  EXPECT_STREQ(new_argv[0], "program_name");
  EXPECT_STREQ(new_argv[1], "-e");
  EXPECT_STREQ(new_argv[2], "require 'fileutils'; FileUtils.copy_entry '/mnt/point', 'source_filesystem'");
}

// Test case 2: With a custom destination argument
TEST(CmdlineArgsTest, extract_custom_destination_arg)
{
  const int argc = 3;
  const char* argv[argc] = {"program_name", "--tebako-extract", "custom_dest"};

  const char* fs_mount_point = "/mnt/point";

  cmdline_args args(argc, argv);
  args.parse_arguments();
  args.build_arguments_for_extract(fs_mount_point);

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 3);
  EXPECT_STREQ(new_argv[0], "program_name");
  EXPECT_STREQ(new_argv[1], "-e");
  EXPECT_STREQ(new_argv[2], "require 'fileutils'; FileUtils.copy_entry '/mnt/point', 'custom_dest'");
}

TEST(CmdlineArgsTest, extract_custom_destination_arg_eq)
{
  const int argc = 2;
  const char* argv[argc] = {"program_name", "--tebako-extract=custom_dest_other"};

  const char* fs_mount_point = "/mnt/point";

  cmdline_args args(argc, argv);
  args.parse_arguments();
  args.build_arguments(fs_mount_point, "/local/test.rb");

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 3);
  EXPECT_STREQ(new_argv[0], "program_name");
  EXPECT_STREQ(new_argv[1], "-e");
  EXPECT_STREQ(new_argv[2], "require 'fileutils'; FileUtils.copy_entry '/mnt/point', 'custom_dest_other'");
}

// Test case 3: Long fs_mount_point input
TEST(CmdlineArgsTest, extract_long_fs_mount_point)
{
  const int argc = 2;
  const char* argv[argc] = {"program_name", "--tebako-extract"};

  std::string long_fs_mount_point(5000, 'a');  // Very long string

  cmdline_args args(argc, argv);
  args.parse_arguments();
  args.build_arguments_for_extract(long_fs_mount_point.c_str());

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 3);
  EXPECT_STREQ(new_argv[0], "program_name");
  EXPECT_STREQ(new_argv[1], "-e");
  std::string expected_cmd =
      "require 'fileutils'; FileUtils.copy_entry '" + long_fs_mount_point + "', 'source_filesystem'";
  EXPECT_STREQ(new_argv[2], expected_cmd.c_str());
}

TEST(CmdlineArgsTest, run_rule)
{
  const int argc = 6;
  const char* argv[argc] = {"program", "--tebako-run", "d1", "--tebako-mount", "d2:m2", "other"};

  cmdline_args args(argc, argv);

  args.parse_arguments();
  auto other_args = args.get_args();
  auto mountpoints = args.get_mountpoints();

  EXPECT_TRUE(args.with_application());
  EXPECT_EQ(args.get_application_image(), "d1");

  // Check that we correctly extracted the tebako-mount rules
  EXPECT_EQ(mountpoints.size(), 1);
  EXPECT_EQ(mountpoints[0], "d2:m2");

  // Check that we correctly placed other arguments
  EXPECT_EQ(other_args.size(), 2);
  EXPECT_EQ(other_args[0], "program");
  EXPECT_EQ(other_args[1], "other");
}

TEST(CmdlineArgsTest, run_rule_eq)
{
  const int argc = 5;
  const char* argv[argc] = {"program", "--tebako-mount", "d2:m2", "other", "--tebako-run=d100"};

  cmdline_args args(argc, argv);

  args.parse_arguments();
  auto other_args = args.get_args();
  auto mountpoints = args.get_mountpoints();

  EXPECT_TRUE(args.with_application());
  EXPECT_EQ(args.get_application_image(), "d100");

  // Check that we correctly extracted the tebako-mount rules
  EXPECT_EQ(mountpoints.size(), 1);
  EXPECT_EQ(mountpoints[0], "d2:m2");

  // Check that we correctly placed other arguments
  EXPECT_EQ(other_args.size(), 2);
  EXPECT_EQ(other_args[0], "program");
  EXPECT_EQ(other_args[1], "other");
}

TEST(CmdlineArgsTest, run_rule_error_on_missing_rule_case_1)
{
  const int argc = 2;
  const char* argv[argc] = {"program", "--tebako-run"};
  cmdline_args args(argc, argv);

  // Expect an exception due to missing rule
  EXPECT_THROW(args.parse_arguments(), std::invalid_argument);
}

TEST(CmdlineArgsTest, run_rule_error_on_missing_rule_case_2)
{
  const int argc = 3;
  const char* argv[argc] = {"program", "--tebako-run", "--other"};
  cmdline_args args(argc, argv);

  // Expect an exception due to missing rule
  EXPECT_THROW(args.parse_arguments(), std::invalid_argument);
}

TEST(CmdlineArgsTest, run_rule_error_on_missing_rule_case_3)
{
  const int argc = 3;
  const char* argv[argc] = {"program", "--tebako-run=", "other"};
  cmdline_args args(argc, argv);

  // Expect an exception due to missing rule
  EXPECT_THROW(args.parse_arguments(), std::invalid_argument);
}

TEST(CmdlineArgsTest, run_rule_error_on_duplicate_rule)
{
  const int argc = 4;
  const char* argv[argc] = {"program", "--tebako-run=d1", "--tebako-run=d2", "other"};
  cmdline_args args(argc, argv);

  // Expect an exception due to duplicate rule
  EXPECT_THROW(args.parse_arguments(), std::invalid_argument);
}

TEST(CmdlineArgsTest, mount_rule)
{
  const int argc = 6;
  const char* argv[argc] = {"program", "--tebako-mount", "d1:m1", "--tebako-mount", "d2:m2", "other"};

  cmdline_args args(argc, argv);

  args.parse_arguments();
  auto other_args = args.get_args();
  auto mountpoints = args.get_mountpoints();

  // Check that we correctly extracted the tebako-mount rules
  EXPECT_EQ(mountpoints.size(), 2);
  EXPECT_EQ(mountpoints[0], "d1:m1");
  EXPECT_EQ(mountpoints[1], "d2:m2");

  // Check that we correctly placed other arguments
  EXPECT_EQ(other_args.size(), 2);
  EXPECT_EQ(other_args[0], "program");
  EXPECT_EQ(other_args[1], "other");
}

TEST(CmdlineArgsTest, mount_rule_with_eq)
{
  const int argc = 5;
  const char* argv[argc] = {"program", "--tebako-mount=d1:m1", "--tebako-mount", "d2:m2", "other"};
  cmdline_args args(argc, argv);

  args.parse_arguments();
  auto other_args = args.get_args();
  auto mountpoints = args.get_mountpoints();

  // Check that we correctly extracted the tebako-mount rules
  EXPECT_EQ(mountpoints.size(), 2);
  EXPECT_EQ(mountpoints[0], "d1:m1");
  EXPECT_EQ(mountpoints[1], "d2:m2");

  // Check that we correctly placed other arguments
  EXPECT_EQ(other_args.size(), 2);
  EXPECT_EQ(other_args[0], "program");
  EXPECT_EQ(other_args[1], "other");
}

TEST(CmdlineArgsTest, mount_throws_error_on_missing_rule)
{
  const int argc = 2;
  const char* argv[argc] = {"program", "--tebako-mount"};
  cmdline_args args(argc, argv);

  // Expect an exception due to missing rule
  EXPECT_THROW(args.parse_arguments(), std::invalid_argument);
}

TEST(CmdlineArgsTest, mount_throws_error_on_missing_rule_after_eq)
{
  const int argc = 2;
  const char* argv[argc] = {"program", "--tebako-mount="};
  cmdline_args args(argc, argv);

  // Expect an exception due to missing rule
  EXPECT_THROW(args.parse_arguments(), std::invalid_argument);
}

TEST(CmdlineArgsTest, mount_throws_error_on_option_in_place_of_rule)
{
  const int argc = 3;
  const char* argv[argc] = {"program", "--tebako-mount", "--option"};
  cmdline_args args(argc, argv);

  // Expect an exception due to missing rule after "="
  EXPECT_THROW(args.parse_arguments(), std::invalid_argument);
}

TEST(CmdlineArgsTest, mount_throws_error_on_on_no_rule)
{
  const int argc = 2;
  const char* argv[argc] = {"program", "--tebako-mount"};
  cmdline_args args(argc, argv);

  // Expect an exception due to missing rule after "="
  EXPECT_THROW(args.parse_arguments(), std::invalid_argument);
}

TEST(CmdlineArgsTest, mount_handles_no_tebako_mount_argument)
{
  const int argc = 3;
  const char* argv[argc] = {"program", "arg1", "arg2"};
  cmdline_args args(argc, argv);

  args.parse_arguments();
  auto other_args = args.get_args();

  // No tebako-mount arguments should be found
  EXPECT_TRUE(args.get_mountpoints().empty());

  // Check that all arguments were placed in other_args
  EXPECT_EQ(other_args.size(), 3);
  EXPECT_EQ(other_args[0], "program");
  EXPECT_EQ(other_args[1], "arg1");
  EXPECT_EQ(other_args[2], "arg2");
}

// Test Case 1: Basic Case
TEST(CmdlineArgsTest, mount_basic_case)
{
  const int argc = 3;
  const char* argv[argc] = {"program", "arg1", "arg2"};
  cmdline_args args(argc, argv);

  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "/local/entry";

  args.parse_arguments();
  args.build_arguments(fs_mount_point, fs_entry_point);

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 4);
  EXPECT_STREQ(new_argv[0], "program");
  EXPECT_STREQ(new_argv[1], "/mnt/local/entry");
  EXPECT_STREQ(new_argv[2], "arg1");
  EXPECT_STREQ(new_argv[3], "arg2");
}

// Test Case 2: No Arguments
TEST(CmdlineArgsTest, mount_no_arguments)
{
  const int argc = 1;
  const char* argv[argc] = {"program"};

  cmdline_args args(argc, argv);

  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "/local/entry";

  args.parse_arguments();
  args.build_arguments(fs_mount_point, fs_entry_point);

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 2);
  EXPECT_STREQ(new_argv[0], "program");
  EXPECT_STREQ(new_argv[1], "/mnt/local/entry");
}

// Test Case 3: Long Strings
TEST(CmdlineArgsTest, mount_long_strings)
{
  std::string arg2 = std::string(1000, 'a'), arg3 = std::string(2000, 'b');

  const int argc = 3;
  const char* argv[argc] = {"program", arg2.c_str(), arg3.c_str()};

  cmdline_args args(argc, argv);

  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "/local/entry";

  args.parse_arguments();
  args.build_arguments(fs_mount_point, fs_entry_point);

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 4);
  EXPECT_STREQ(new_argv[0], "program");
  EXPECT_STREQ(new_argv[1], "/mnt/local/entry");
  EXPECT_EQ(std::string(new_argv[2]), std::string(1000, 'a'));
  EXPECT_EQ(std::string(new_argv[3]), std::string(2000, 'b'));
}

// Test Case 4: Special Characters
TEST(CmdlineArgsTest, mount_special_characters)
{
  const int argc = 4;
  const char* argv[argc] = {"program", "arg 1", "arg\t2", "arg@3"};

  cmdline_args args(argc, argv);

  const char* fs_mount_point = "/mnt";
  const char* fs_entry_point = "/lcal/entry";

  args.parse_arguments();
  args.build_arguments(fs_mount_point, fs_entry_point);

  int new_argc = args.get_argc();
  char** new_argv = args.get_argv();

  EXPECT_EQ(new_argc, 5);
  EXPECT_STREQ(new_argv[0], "program");
  EXPECT_STREQ(new_argv[1], "/mnt/lcal/entry");
  EXPECT_STREQ(new_argv[2], "arg 1");
  EXPECT_STREQ(new_argv[3], "arg\t2");
  EXPECT_STREQ(new_argv[4], "arg@3");
}

// Test Case 5: Empty Mount Point and Entry Point
TEST(CmdlineArgsTest, mount_empty_mount_or_entry_points)
{
  const int argc = 2;
  const char* argv[argc] = {"program", "arg1"};
  cmdline_args args(argc, argv);

  args.parse_arguments();

  EXPECT_THROW(args.build_arguments("", "/lcal/entry"), std::invalid_argument);
  EXPECT_THROW(args.build_arguments("/mnt", ""), std::invalid_argument);
}

// Test Case 6: Null Mount or Entry Point
TEST(CmdlineArgsTest, mount_null_mount_or_entry_points)
{
  const int argc = 2;
  const char* argv[argc] = {"program", "arg1"};
  cmdline_args args(argc, argv);

  args.parse_arguments();

  EXPECT_THROW(args.build_arguments(nullptr, "/entry"), std::invalid_argument);
  EXPECT_THROW(args.build_arguments("/mnt", nullptr), std::invalid_argument);
}

TEST(CmdlineArgsTest, process_package_descriptor)
{
  const int argc = 2;
  std::string pkg = std::string("--tebako-run=") + tests_the_other_memfs_image();
  const char* argv[argc] = {"program", pkg.c_str()};
  cmdline_args args(argc, argv);

  args.parse_arguments();

  EXPECT_TRUE(args.with_application());
  EXPECT_EQ(args.get_application_image(), tests_the_other_memfs_image());

  args.process_package();

  auto descriptor = args.get_descriptor();

  EXPECT_TRUE(descriptor.has_value());
  EXPECT_EQ(descriptor->get_ruby_version_major(), 3);
  EXPECT_EQ(descriptor->get_ruby_version_minor(), 2);
  EXPECT_EQ(descriptor->get_ruby_version_patch(), 5);
  EXPECT_EQ(descriptor->get_tebako_version_major(), 0);
  EXPECT_EQ(descriptor->get_tebako_version_minor(), 10);
  EXPECT_EQ(descriptor->get_tebako_version_patch(), 1);
  EXPECT_EQ(descriptor->get_mount_point(), "/__tebako_memfs__");
  EXPECT_EQ(descriptor->get_entry_point(), "/local/tebako-test-run.rb");
  EXPECT_FALSE(descriptor->get_cwd().has_value());
}

}  // namespace tebako