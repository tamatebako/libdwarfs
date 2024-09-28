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

  delete[] pp[0];
  delete p;

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

  delete[] pp[0];
  delete[] pp[1];
  delete[] pp[2];
  delete p;

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

  delete[] pp[0];
  delete p;

  delete[] argv[0];
  delete[] argv;
}
}  // namespace tebako