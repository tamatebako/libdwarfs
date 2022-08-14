/**
 *
 * Copyright (c) 2021-2022, [Ribose Inc](https://www.ribose.com).
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

#include <unistd.h>
#include <filesystem>
#include "tests.h"

namespace fs = std::filesystem;

namespace {
	class DirCtlTests : public testing::Test {
	protected:
		static std::string tmp_dir;
		static std::string tmp_name;
// #define -- for TEBAKIZE_PATH
#define TMP_D_NAME "tebako-test-dir"

		static void SetUpTestSuite() {

#ifdef RB_W32
			do_rb_w32_init();
#endif

			auto p_tmp_dir = fs::temp_directory_path();
			auto p_tmp_name = p_tmp_dir / TMP_D_NAME;

			tmp_dir = p_tmp_dir.generic_string();
			tmp_name = p_tmp_name.generic_string();

			load_fs(&gfsData[0],
				gfsSize,
				tests_log_level,
				NULL	/* cachesize*/,
				NULL	/* workers */,
				NULL	/* mlock */,
				NULL	/* decompress_ratio*/,
				NULL    /* image_offset */
			);

		}

		static void TearDownTestSuite() {
			drop_fs();
		}
	};

	TEST_F(DirCtlTests, tebako_chdir_absolute_path_getcwd) {
		char p[PATH_MAX];
		char* r;
		int ret = tebako_chdir(TEBAKIZE_PATH(""));
		EXPECT_EQ(0, ret);
		r = tebako_getcwd(p, PATH_MAX);
		EXPECT_STREQ(r, TEBAKIZE_PATH(""));
	}

	TEST_F(DirCtlTests, tebako_chdir_absolute_path_no_directory_getcwd) {
		char p1[PATH_MAX], p2[PATH_MAX];
		char *r1, *r2;
		r1 = tebako_getcwd(p1, PATH_MAX);
		int ret = tebako_chdir(TEBAKIZE_PATH("no-directory"));
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
		r2 = tebako_getcwd(p2, PATH_MAX);
		EXPECT_STREQ(r1, r2);
	}

	TEST_F(DirCtlTests, tebako_getcwd_small_buffer) {
		char p1[PATH_MAX], p2[10];
		char* r2;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-1"));
		EXPECT_EQ(0, ret);
		r2 = tebako_getcwd(p2, sizeof(p2)/sizeof(p2[0]));
		EXPECT_EQ(ERANGE, errno);
		EXPECT_EQ(NULL, r2);
	}

	TEST_F(DirCtlTests, tebako_getcwd_no_buffer_size) {
		char p1[PATH_MAX];
		char* r2;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		r2 = tebako_getcwd(NULL, PATH_MAX);
		EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-2" __S__));
		free(r2);
	}

	TEST_F(DirCtlTests, tebako_getcwd_no_buffer_no_size) {
		char p1[PATH_MAX];
		char* r2;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-1"));
		EXPECT_EQ(0, ret);
		r2 = tebako_getcwd(NULL, 0);
		EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-1" __S__));
		free(r2);
	}

	TEST_F(DirCtlTests, tebako_getcwd_buffer_no_size) {
		char p1[PATH_MAX], p2[PATH_MAX];
		char* r2;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-1"));
		EXPECT_EQ(0, ret);
		r2 = tebako_getcwd(p2, 0);
		EXPECT_EQ(EINVAL, errno);
		EXPECT_EQ(NULL, r2);
	}
	TEST_F(DirCtlTests, tebako_chdir_absolute_path_pass_through) {
		int ret = tebako_chdir(__BIN__);
		EXPECT_EQ(0, ret);
	}

	TEST_F(DirCtlTests, tebako_chdir_relative_path_pass_through) {
		int ret = tebako_chdir(__MSYS_USR__ __S__);
		EXPECT_EQ(0, ret);
		ret = tebako_chdir("bin");
		EXPECT_EQ(0, ret);
	}

	TEST_F(DirCtlTests, tebako_mkdir_absolute_path) {
#if defined(TEBAKO_HAS_POSIX_MKDIR) || defined(RB_W32)
		int ret = tebako_mkdir(TEBAKIZE_PATH(TMP_D_NAME), S_IRWXU);
#else
		int ret = tebako_mkdir(TEBAKIZE_PATH(TMP_D_NAME));
#endif
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EROFS, errno);
	}

	TEST_F(DirCtlTests, tebako_mkdir_relative_path) {
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

	TEST_F(DirCtlTests, tebako_mkdir_absolute_path_pass_through) {
#if defined(TEBAKO_HAS_POSIX_MKDIR) || defined(RB_W32)
		int ret = tebako_mkdir(tmp_name.c_str(), S_IRWXU);
#else
		int ret = tebako_mkdir(tmp_name.c_str());
#endif
		EXPECT_EQ(0, ret);
		ret = rmdir(tmp_name.c_str());
		EXPECT_EQ(0, ret);
	}

	TEST_F(DirCtlTests, tebako_mkdir_relative_path_pass_through) {
		int ret = tebako_chdir(tmp_dir.c_str());
		EXPECT_EQ(0, ret);
#if defined(TEBAKO_HAS_POSIX_MKDIR) || defined(RB_W32)
		ret = tebako_mkdir(TMP_D_NAME, S_IRWXU);
#else
		ret = tebako_mkdir(TMP_D_NAME);
#endif
		EXPECT_EQ(0, ret);
		ret = rmdir(tmp_name.c_str());
		EXPECT_EQ(0, ret);
	}

	TEST_F(DirCtlTests, tebako_dir_ctl_null_ptr) {
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
	}

	TEST_F(DirCtlTests, tebako_dir_ctl_dot_dot) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-3/level-1/level-2/level-3/level-4/../.."));
		EXPECT_EQ(0, ret);

		char* r2 = tebako_getcwd(NULL, 0);
		EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-3" __S__ "level-1" __S__ "level-2" __S__));
		if (r2) {
			free(r2);
		}
	}

	TEST_F(DirCtlTests, tebako_dir_ctl_root) {
		//  "/__tebako_memfs__" and not conventional "/__tebako_memfs__/"
		int ret = tebako_chdir("/__tebako_memfs__");
		EXPECT_EQ(0, ret);

		char* r2 = tebako_getcwd(NULL, 0);
		EXPECT_STREQ(r2, TEBAKIZE_PATH(""));
		if (r2) {
			free(r2);
		}
	}

	std::string DirCtlTests::tmp_name;
	std::string DirCtlTests::tmp_dir;
}
