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
#include <tebako-io-inner.h>
#include <tebako-mnt.h>
#include <tebako-cmdline-helpers.h>

namespace tebako {

// Test cases
class ProcessMountpointsTest : public ::testing::Test {
 protected:
  struct stat st;
  static uint32_t test_root_ino;

 protected:
#ifdef _WIN32
  static void invalidParameterHandler(const wchar_t* p1,
                                      const wchar_t* p2,
                                      const wchar_t* p3,
                                      unsigned int p4,
                                      uintptr_t p5)
  {
    // Just return to pass execution to standard library
    // otherwise exception will be thrown by MSVC runtime
    return;
  }
#endif

  static void SetUpTestSuite()
  {
#ifdef _WIN32
    _set_invalid_parameter_handler(invalidParameterHandler);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif

    load_fs(&gfsData[0], gfsSize, tests_log_level(), NULL /* cachesize*/, NULL /* workers */, NULL /* mlock */,
            NULL /* decompress_ratio*/, NULL /* image_offset */
    );

    std::string lnk;
    struct stat st;
    int res = dwarfs_stat(TEBAKIZE_PATH("directory-1"), &st, lnk, false);
    EXPECT_EQ(DWARFS_IO_CONTINUE, res);
    test_root_ino = st.st_ino;
  }

  void SetUp() override
  {
    // Clear the mount table before each test
    sync_tebako_mount_table::get_tebako_mount_table().clear();
  }
};

uint32_t ProcessMountpointsTest::test_root_ino;

// Test: Valid mount point
TEST_F(ProcessMountpointsTest, valid_mountpoint)
{
  std::vector<std::string> mountpoints = {"directory-1/mpoint:/tmp/local_target"};
  EXPECT_NO_THROW(process_mountpoints(mountpoints));
  EXPECT_TRUE(sync_tebako_mount_table::get_tebako_mount_table().check(test_root_ino, "mpoint"));
}

TEST_F(ProcessMountpointsTest, absolute_path)
{
  std::vector<std::string> mountpoints = {"/directory-1/mpoint:/tmp/local_target"};
  EXPECT_THROW(process_mountpoints(mountpoints), std::invalid_argument);
}

// Test: Missing ':' separator
TEST_F(ProcessMountpointsTest, missing_separator)
{
  std::vector<std::string> mountpoints = {"directory-1/mpoint"};
  EXPECT_THROW(process_mountpoints(mountpoints), std::invalid_argument);
}

// Test: Empty path or filename
TEST_F(ProcessMountpointsTest, empty_path)
{
  std::vector<std::string> mountpoints = {":/tmp/local_target"};
  EXPECT_THROW(process_mountpoints(mountpoints), std::invalid_argument);
}

// Test: Empty target
TEST_F(ProcessMountpointsTest, empty_target)
{
  std::vector<std::string> mountpoints = {"directory-1/mpoint:"};
  EXPECT_THROW(process_mountpoints(mountpoints), std::invalid_argument);
}

// Test: Invalid path (dwarfs_stat returns error)
TEST_F(ProcessMountpointsTest, invalid_path)
{
  std::vector<std::string> mountpoints = {"invalid/path:local_target"};
  EXPECT_THROW(process_mountpoints(mountpoints), std::invalid_argument);
}

// Test: Multiple valid mount points
TEST_F(ProcessMountpointsTest, MultipleValidMountPoints)
{
  std::vector<std::string> mountpoints = {"directory-1/mpoint1:target1", "directory-1/mpoint2:target2",
                                          "directory-1/mpoint3:target3"};

  EXPECT_NO_THROW(process_mountpoints(mountpoints));

  EXPECT_TRUE(sync_tebako_mount_table::get_tebako_mount_table().check(test_root_ino, "mpoint1"));
  EXPECT_TRUE(sync_tebako_mount_table::get_tebako_mount_table().check(test_root_ino, "mpoint2"));
  EXPECT_TRUE(sync_tebako_mount_table::get_tebako_mount_table().check(test_root_ino, "mpoint3"));
}

}  // namespace tebako
