/**
 *
 * Copyright (c) 2021-2024, [Ribose Inc](https://www.ribose.com).
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

#include <filesystem>
namespace fs = std::filesystem;
#include "tests.h"

namespace {
// This is the directory name for mkdir attempts within tebako memefs (those, that will fail)
// #define -- for TEBAKIZE_PATH
#define TMP_D_NAME "tebako-test-dir"

class DirCtlTests : public testing::Test {
 protected:
  // This is the directory name for mkdir attempts outside of tebako memefs (those, that may succeed)
  std::string tmp_dir;
  std::string tmp_name;

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
#endif
    mount_root_memfs(&gfsData[0], gfsSize, tests_log_level(), NULL /* cachesize*/, NULL /* workers */, NULL /* mlock */,
                     NULL /* decompress_ratio*/, NULL /* image_offset */
    );
  }

  static void TearDownTestSuite()
  {
    unmount_root_memfs();
  }

  void SetUp() override
  {
    auto p_tmp_dir = stdfs::temp_directory_path();
    // We will assume that this file does not exist ...
    auto p_tmp_name = p_tmp_dir / (TMP_D_NAME + generateRandom6DigitNumber());

    tmp_dir = p_tmp_dir.generic_string();
    tmp_name = p_tmp_name.generic_string();
  }

  std::string generateRandom6DigitNumber()
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    int random_number = dis(gen);
    return std::to_string(random_number);
  }
};

TEST_F(DirCtlTests, tebako_chdir_absolute_path_getcwd)
{
  char p[PATH_MAX];
  char* r;
  int ret = tebako_chdir(TEBAKIZE_PATH(""));
  EXPECT_EQ(0, ret);
  r = tebako_getcwd(p, PATH_MAX);
  EXPECT_STREQ(r, TEBAKIZE_PATH(""));
  ret = is_tebako_cwd();
  EXPECT_NE(0, ret);
}

TEST_F(DirCtlTests, tebako_chdir_absolute_path_no_directory_getcwd)
{
  char p1[PATH_MAX], p2[PATH_MAX];
  char *r1, *r2;
  r1 = tebako_getcwd(p1, PATH_MAX);
  int ret = tebako_chdir(TEBAKIZE_PATH("no-directory"));
  EXPECT_EQ(ENOENT, errno);
  EXPECT_EQ(-1, ret);
  r2 = tebako_getcwd(p2, PATH_MAX);
  EXPECT_STREQ(r1, r2);
}

TEST_F(DirCtlTests, tebako_getcwd_small_buffer)
{
  char p1[PATH_MAX], p2[10];
  char* r2;
  int ret = tebako_chdir(TEBAKIZE_PATH("directory-1"));
  EXPECT_EQ(0, ret);

  r2 = tebako_getcwd(p2, sizeof(p2) / sizeof(p2[0]));
  EXPECT_EQ(ERANGE, errno);
  EXPECT_EQ(NULL, r2);

  r2 = tebako_getcwd(nullptr, sizeof(p2) / sizeof(p2[0]));
  EXPECT_EQ(ERANGE, errno);
  EXPECT_EQ(NULL, r2);
}

TEST_F(DirCtlTests, tebako_getcwd_no_buffer_size)
{
  char p1[PATH_MAX];
  char* r2;
  int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
  EXPECT_EQ(0, ret);
  r2 = tebako_getcwd(NULL, PATH_MAX);
  EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-2" __S__));
  free(r2);
}

TEST_F(DirCtlTests, tebako_getcwd_no_buffer_no_size)
{
  char p1[PATH_MAX];
  char* r2;
  int ret = tebako_chdir(TEBAKIZE_PATH("directory-1"));
  EXPECT_EQ(0, ret);
  r2 = tebako_getcwd(NULL, 0);
  EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-1" __S__));
  free(r2);
}

TEST_F(DirCtlTests, tebako_getcwd_buffer_no_size)
{
  char p1[PATH_MAX], p2[PATH_MAX];
  char* r2;
  int ret = tebako_chdir(TEBAKIZE_PATH("directory-1"));
  EXPECT_EQ(0, ret);
  r2 = tebako_getcwd(p2, 0);
  EXPECT_EQ(EINVAL, errno);
  EXPECT_EQ(NULL, r2);
}
TEST_F(DirCtlTests, tebako_chdir_absolute_path_pass_through)
{
  int ret = tebako_chdir(__BIN__);
  EXPECT_EQ(0, ret);
  ret = is_tebako_cwd();
  EXPECT_EQ(0, ret);
}

TEST_F(DirCtlTests, tebako_chdir_relative_path_pass_through)
{
  int ret = tebako_chdir(__USR__ __S__);
  EXPECT_EQ(0, ret);
  ret = tebako_chdir(__USR_NEXT__);
  EXPECT_EQ(0, ret);
}

TEST_F(DirCtlTests, tebako_mkdir_absolute_path)
{
#if defined(TEBAKO_HAS_POSIX_MKDIR) || defined(RB_W32)
  int ret = tebako_mkdir(TEBAKIZE_PATH(TMP_D_NAME), S_IRWXU);
#else
  int ret = tebako_mkdir(TEBAKIZE_PATH(TMP_D_NAME));
#endif
  EXPECT_EQ(-1, ret);
  EXPECT_EQ(EROFS, errno);
}

TEST_F(DirCtlTests, tebako_rmdir_erofs)
{
  int ret = tebako_rmdir(TEBAKIZE_PATH("directory-1"));
  EXPECT_EQ(-1, ret);
  EXPECT_EQ(EROFS, errno);
}

TEST_F(DirCtlTests, tebako_unlink_erofs)
{
  int ret = tebako_unlink(TEBAKIZE_PATH("file-1.txt"));
  EXPECT_EQ(-1, ret);
  EXPECT_EQ(EROFS, errno);
}

TEST_F(DirCtlTests, tebako_mkdir_relative_path)
{
  int ret = tebako_chdir(TEBAKIZE_PATH(""));
  EXPECT_EQ(0, ret);
#if defined(TEBAKO_HAS_POSIX_MKDIR) || defined(RB_W32)
  ret = tebako_mkdir(TMP_D_NAME, S_IRWXU);
#else
  ret = tebako_mkdir(TMP_D_NAME);
#endif
  EXPECT_EQ(-1, ret);
  EXPECT_EQ(EROFS, errno);
}

TEST_F(DirCtlTests, tebako_mkdir_absolute_path_pass_through)
{
#if defined(TEBAKO_HAS_POSIX_MKDIR) || defined(RB_W32)
  int ret = tebako_mkdir(tmp_name.c_str(), S_IRWXU);
#else
  int ret = tebako_mkdir(tmp_name.c_str());
#endif
  EXPECT_EQ(0, ret);
  ret = tebako_rmdir(tmp_name.c_str());
  EXPECT_EQ(0, ret);
}

TEST_F(DirCtlTests, tebako_mkdir_relative_path_pass_through)
{
  int ret = tebako_chdir(tmp_dir.c_str());
  EXPECT_EQ(0, ret);
#if defined(TEBAKO_HAS_POSIX_MKDIR) || defined(RB_W32)
  ret = tebako_mkdir(tmp_name.c_str(), S_IRWXU);
#else
  ret = tebako_mkdir(tmp_name.c_str());
#endif
  EXPECT_EQ(0, ret);
  ret = tebako_rmdir(tmp_name.c_str());
  EXPECT_EQ(0, ret);
}

TEST_F(DirCtlTests, tebako_dir_ctl_null_ptr)
{
  errno = 0;
  EXPECT_EQ(-1, tebako_chdir(NULL));
  EXPECT_EQ(ENOENT, errno);

  errno = 0;
#if defined(TEBAKO_HAS_POSIX_MKDIR) || defined(RB_W32)
  EXPECT_EQ(-1, tebako_mkdir(NULL, S_IRWXU));
#else
  EXPECT_EQ(-1, tebako_mkdir(NULL));
#endif
  EXPECT_EQ(ENOENT, errno);

  EXPECT_EQ(-1, tebako_rmdir(NULL));
  EXPECT_EQ(ENOENT, errno);

  EXPECT_EQ(-1, tebako_unlink(NULL));
  EXPECT_EQ(ENOENT, errno);
}

TEST_F(DirCtlTests, tebako_dir_ctl_dot_dot)
{
  int ret = tebako_chdir(TEBAKIZE_PATH("directory-3/level-1/level-2/level-3/level-4/../.."));
  EXPECT_EQ(0, ret);

  char* r2 = tebako_getcwd(NULL, 0);
  EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-3" __S__ "level-1" __S__ "level-2" __S__));
  if (r2) {
    free(r2);
  }
}

TEST_F(DirCtlTests, tebako_dir_ctl_root)
{
  //  "/__tebako_memfs__" and not conventional "/__tebako_memfs__/"
#ifdef _WIN32
  int ret = tebako_chdir("A:\\__tebako_memfs__");
#else
  int ret = tebako_chdir("/__tebako_memfs__");
#endif
  EXPECT_EQ(0, ret);

  char* r2 = tebako_getcwd(NULL, 0);
  EXPECT_STREQ(r2, TEBAKIZE_PATH(""));
  if (r2) {
    free(r2);
  }
}

#ifdef _WIN32
TEST_F(DirCtlTests, is_tebako_path_w)
{
  int ret = is_tebako_path_w(L"A:/__tebako_memfs__/some/path");
  EXPECT_EQ(-1, ret);

  ret = is_tebako_path_w(L"A:\\__tebako_memfs__\\some\\path");
  EXPECT_EQ(-1, ret);

  ret = is_tebako_path_w(L"/just/some/path");
  EXPECT_EQ(0, ret);

  ret = is_tebako_path_w(L"C:\\just\\some\\path");
  EXPECT_EQ(0, ret);
}
#endif
}  // namespace
