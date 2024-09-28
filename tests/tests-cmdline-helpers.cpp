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

  int result = tebako_extract_cmdline(&argc, &argv, fs_mount_point);

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

  int result = tebako_extract_cmdline(&argc, &argv, fs_mount_point);

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

  int result = tebako_extract_cmdline(&argc, &argv, long_fs_mount_point.c_str());

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

  auto [tebako_mount_args, other_args] = tebako_parse_arguments(argc, argv);

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
  EXPECT_THROW(tebako_parse_arguments(argc, argv), std::invalid_argument);
}

TEST(ProcessArgumentsTest, throws_error_on_option_in_place_of_rule)
{
  const char* argv[] = {"program", "--tebako-mount", "--option"};
  int argc = 3;

  // Expect an exception due to missing rule after "="
  EXPECT_THROW(tebako_parse_arguments(argc, argv), std::invalid_argument);
}

TEST(ProcessArgumentsTest, throws_error_on_on_no_rule)
{
  const char* argv[] = {"program", "--tebako-mount"};
  int argc = 2;

  // Expect an exception due to missing rule after "="
  EXPECT_THROW(tebako_parse_arguments(argc, argv), std::invalid_argument);
}

TEST(ProcessArgumentsTest, handles_no_tebako_mount_argument)
{
  const char* argv[] = {"program", "arg1", "arg2"};
  int argc = 3;

  auto [tebako_mount_args, other_args] = tebako_parse_arguments(argc, argv);

  // No tebako-mount arguments should be found
  EXPECT_TRUE(tebako_mount_args.empty());

  // Check that all arguments were placed in other_args
  EXPECT_EQ(other_args.size(), 3);
  EXPECT_EQ(other_args[0], "program");
  EXPECT_EQ(other_args[1], "arg1");
  EXPECT_EQ(other_args[2], "arg2");
}

}  // namespace tebako