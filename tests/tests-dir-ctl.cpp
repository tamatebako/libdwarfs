/**
 *
 * Copyright (c) 2021, [Ribose Inc](https://www.ribose.com).
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
#include "tests.h"

 /*
 *  Unit tests for 'tebako_getcwd', 'tebako_getwd', 'tebako_chdir', 'tebako_mkdir' functions
 */

namespace {
	class DirCtlTests : public testing::Test {
	protected:
		static void SetUpTestSuite() {
			load_fs(&gfsData[0],
				gfsSize,
				"warn" /*debuglevel*/,
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
		EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-2"));
		free(r2);
	}

	TEST_F(DirCtlTests, tebako_getcwd_no_buffer_no_size) {
		char p1[PATH_MAX];
		char* r2;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-1"));
		EXPECT_EQ(0, ret);
		r2 = tebako_getcwd(NULL, 0);
		EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-1"));
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

	TEST_F(DirCtlTests, tebako_chdir_relative_path_getwd) {
		char p[PATH_MAX];
		char* r;
		int ret = tebako_chdir(TEBAKIZE_PATH(""));
		EXPECT_EQ(0, ret);
		ret = tebako_chdir("directory-2");
		EXPECT_EQ(0, ret);
		r = tebako_getwd(p);
		EXPECT_STREQ(r, TEBAKIZE_PATH("directory-2"));
	}

	TEST_F(DirCtlTests, tebako_chdir_relative_path_no_directory_getwd) {
		char p1[PATH_MAX], p2[PATH_MAX];
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-1"));
		EXPECT_EQ(0, ret);
		ret = tebako_chdir("no-directory");
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(DirCtlTests, tebako_chdir_absolute_path_pass_through) {
		int ret = tebako_chdir("/usr/bin");
		EXPECT_EQ(0, ret);
	}

	TEST_F(DirCtlTests, tebako_chdir_relative_path_pass_through) {
		int ret = tebako_chdir("/usr/");
		EXPECT_EQ(0, ret);
		ret = tebako_chdir("bin");
		EXPECT_EQ(0, ret);
	}

	TEST_F(DirCtlTests, tebako_mkdir_absolute_path) {
		int ret = tebako_mkdir(TEBAKIZE_PATH("tebako-test-dir"), S_IRWXU);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EROFS, errno);
	}

	TEST_F(DirCtlTests, tebako_mkdir_relative_path) {
		int ret = tebako_chdir(TEBAKIZE_PATH(""));
		EXPECT_EQ(0, ret);
		ret = tebako_mkdir("tebako-test-dir", S_IRWXU);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EROFS, errno);
	}

	TEST_F(DirCtlTests, tebako_mkdir_absolute_path_pass_through) {
		int ret = tebako_mkdir("/tmp/tebako-test-dir", S_IRWXU);
		EXPECT_EQ(0, ret);
		ret = rmdir("/tmp/tebako-test-dir");
		EXPECT_EQ(0, ret);
	}

	TEST_F(DirCtlTests, tebako_mkdir_relative_path_pass_through) {
		int ret = tebako_chdir("/tmp");
		EXPECT_EQ(0, ret);
		ret = tebako_mkdir("tebako-test-dir", S_IRWXU);
		EXPECT_EQ(0, ret);
		ret = rmdir("/tmp/tebako-test-dir");
		EXPECT_EQ(0, ret);
	}

	TEST_F(DirCtlTests, tebako_dir_ctl_null_ptr) {
		errno = 0;
		EXPECT_EQ(-1, tebako_chdir(NULL));
		EXPECT_EQ(ENOENT, errno);

		errno = 0;
		EXPECT_EQ(-1, tebako_mkdir(NULL, S_IRWXU));
		EXPECT_EQ(ENOENT, errno);

		errno = 0;
		EXPECT_EQ(NULL, tebako_getwd(NULL));
		EXPECT_EQ(ENOENT, errno);
	}

	TEST_F(DirCtlTests, tebako_dir_ctl_dot_dot) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-3/level-1/level-2/level-3/level-4/../.."));
		EXPECT_EQ(0, ret);

		char* r2 = tebako_getcwd(NULL, 0);
		EXPECT_STREQ(r2, TEBAKIZE_PATH("directory-3/level-1/level-2/"));
		if (r2) {
			free(r2);
		}

	}

}
