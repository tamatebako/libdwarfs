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

#include "tests.h"

namespace {
	class FileCtlTests : public testing::Test {
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

	TEST_F(FileCtlTests, tebako_access_absolute_path) {
		int ret = tebako_access(TEBAKIZE_PATH("file.txt"), F_OK);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_access_absolute_path_no_file) {
		int ret = tebako_access(TEBAKIZE_PATH("no-directory/file.txt"), W_OK);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(FileCtlTests, tebako_access_null) {
		int ret = tebako_access(NULL, W_OK);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(FileCtlTests, tebako_access_relative_path) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_access("file-in-directory-2.txt", R_OK);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_access_relative_path_no_file) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_access("no-file-in-directory-2.txt", R_OK);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(FileCtlTests, tebako_access_absolute_path_pass_through) {
		int ret = tebako_access("/bin/bash", F_OK);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_access_relative_path_pass_through) {
		int ret = tebako_chdir("/usr/");
		EXPECT_EQ(0, ret);
		ret = tebako_access("bin", R_OK|X_OK);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_access_absolute_path_no_access) {
		int ret = tebako_access(TEBAKIZE_PATH("restricted-do-not-touch.txt"), W_OK);
		EXPECT_EQ(EACCES, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(FileCtlTests, tebako_stat_absolute_path) {
		struct stat st;
		int ret = tebako_stat(TEBAKIZE_PATH("file.txt"), &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_stat_absolute_path_no_file) {
		struct stat st;
		int ret = tebako_stat(TEBAKIZE_PATH("no_file.txt"), &st);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(FileCtlTests, tebako_stat_null) {
		struct stat st;
		int ret = tebako_stat(NULL, &st);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(FileCtlTests, tebako_stat_relative_path) {
		struct stat st;
		int ret = tebako_chdir(TEBAKIZE_PATH(""));
		EXPECT_EQ(0, ret);
		ret = tebako_stat("directory-1/file-in-directory-1.txt", &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_stat_relative_path_no_file) {
		struct stat st;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_stat("no_file.txt", &st);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(FileCtlTests, tebako_stat_absolute_path_pass_through) {
		struct stat st;
		int ret = tebako_stat("/bin/bash", &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_stat_relative_path_pass_through) {
		struct stat st;
		int ret = tebako_chdir("/bin");
		EXPECT_EQ(0, ret);
		ret = tebako_stat("sh", &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_open_fstat_close_absolute_path) {
		int fh = tebako_open(2, TEBAKIZE_PATH("directory-1/file-in-directory-1.txt"), O_RDONLY);
		EXPECT_LT(0, fh);

		struct stat st;
		int ret = tebako_fstat(fh, &st);
		EXPECT_EQ(0, ret);

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_open_fstat_close_relative_path) {
		struct stat buf;
		int ret = tebako_chdir(TEBAKIZE_PATH(""));
		EXPECT_EQ(0, ret);
		int fh = tebako_open(2, "directory-1/file-in-directory-1.txt", O_RDONLY);
		EXPECT_LT(0, fh);
		ret = tebako_fstat(fh, &buf);
		EXPECT_EQ(0, ret);
		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_open_fstat_close_absolute_path_pass_through) {
		int fh = tebako_open(2, "/bin/bash", O_RDONLY);
		EXPECT_LT(0, fh);

		struct stat st;
		int ret = tebako_fstat(fh, &st);
		EXPECT_EQ(0, ret);

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_open_fstat_close_relative_path_pass_through) {
		struct stat st;
		int ret = tebako_chdir("/");
		EXPECT_EQ(0, ret);
		int fh = tebako_open(2, "bin/bash", O_RDONLY);
		EXPECT_LT(0, fh);
		ret = tebako_fstat(fh, &st);
		EXPECT_EQ(0, ret);
		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_fstatat_relative_path) {
		struct stat buf;
		int fd = tebako_open(2, TEBAKIZE_PATH("directory-1"), O_RDONLY | O_DIRECTORY);
		EXPECT_LT(0, fd);
		int ret = tebako_fstatat(fd, "file-in-directory-1.txt", &buf, 0);
		EXPECT_EQ(0, ret);
		ret = tebako_close(fd);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_fstatat_absolute_path) {
		struct stat buf;
		int fd = tebako_open(2, TEBAKIZE_PATH("directory-1"), O_RDONLY | O_DIRECTORY);
		EXPECT_LT(0, fd);
		int ret = tebako_fstatat(fd, TEBAKIZE_PATH("directory-1/file-in-directory-1.txt"), &buf, 0);
		EXPECT_EQ(0, ret);
		ret = tebako_close(fd);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_fstatat_relative_path_pass_through) {
		struct stat buf;
		int fd = tebako_open(2, "/bin", O_RDONLY | O_DIRECTORY);
		EXPECT_LT(0, fd);
		int ret = tebako_fstatat(fd, "bash", &buf, 0);
		EXPECT_EQ(0, ret);
		ret = tebako_close(fd);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_fstatat_absolute_path_pass_through) {
		struct stat buf;
		int fd = tebako_open(2, TEBAKIZE_PATH("directory-1"), O_RDONLY | O_DIRECTORY);
		EXPECT_LT(0, fd);
		int ret = tebako_fstatat(fd, "bin/bash", &buf, 0);
		EXPECT_EQ(0, ret);
		ret = tebako_close(fd);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_fstatat_at_fdcwd) {
		struct stat buf;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_fstatat(AT_FDCWD, "file-in-directory-2.txt", &buf, 0);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileCtlTests, tebako_access_relative_path_dot_dot) {
		int ret = tebako_chdir(TEBAKIZE_PATH("//directory-3/level-1//"));
		EXPECT_EQ(0, ret);
		ret = tebako_access("level-2/../../../directory-2/file-in-directory-2.txt", R_OK);
		EXPECT_EQ(0, ret);
	}

}
