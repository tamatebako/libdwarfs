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
#include <gnu/lib-names.h> 

namespace {
	class LnTests : public testing::Test {
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
#ifdef WITH_LINK_TESTS
	TEST_F(LnTests, tebako_softlink) {
		int fh = tebako_open(2, TEBAKIZE_PATH("s-link-to-dir-1"), O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);

		int ret = tebako_read(fh, readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_EQ(0, strncmp(readbuf, "This is a file in the first directory", num2read));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(LnTests, tebako_hardlink) {
		int fh = tebako_open(2, TEBAKIZE_PATH("h-link-to-dir-2"), O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[32];
		const int num2read = sizeof(readbuf)/sizeof(readbuf[0]);

		int ret = tebako_read(fh, readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_EQ(0, strncmp(readbuf, "This is a file in the second directory", num2read));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(LnTests, tebako_readlink_absolute_path_no_file) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		int ret = tebako_readlink(TEBAKIZE_PATH("no_file"), readbuf, num2read);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(LnTests, tebako_readlink_null) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		int ret = tebako_readlink(NULL, readbuf, num2read);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(LnTests, tebako_readlink_not_link) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		int ret = tebako_readlink(TEBAKIZE_PATH("file.txt"), readbuf, num2read);
		EXPECT_EQ(EINVAL, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(LnTests, tebako_readlink_relative_path_no_file) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_readlink("no_file", readbuf, num2read);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(LnTests, tebako_readlink_absolute_path) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		int ret = tebako_readlink(TEBAKIZE_PATH("s-link-to-dir-1"), readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_TRUE(strncmp(readbuf, "directory-1/file-in-directory-1.txt", num2read) == 0);
	}

	TEST_F(LnTests, tebako_readlink_relative_path) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		EXPECT_EQ(0, tebako_chdir(TEBAKIZE_PATH("")));
		int ret = tebako_readlink("s-link-to-dir-1", readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_TRUE(strncmp(readbuf, "directory-1/file-in-directory-1.txt", num2read) == 0);
	}

	TEST_F(LnTests, tebako_readlink_absolute_path_pass_through) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		int ret = tebako_readlink("/bin/sh", readbuf, num2read);
		EXPECT_EQ(strlen("dash"), ret);
		EXPECT_TRUE(strncmp(readbuf, "dash", num2read) == 0);
	}

	TEST_F(LnTests, tebako_readlink_relative_path_pass_through) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		EXPECT_EQ(0, tebako_chdir("/bin"));
		int ret = tebako_readlink("sh", readbuf, num2read);
		EXPECT_EQ(strlen("dash"), ret);
		EXPECT_TRUE(strncmp(readbuf, "dash", num2read) == 0);
	}

	TEST_F(LnTests, tebako_lstat_absolute_path) {
		struct stat st;
		int ret = tebako_lstat(TEBAKIZE_PATH("s-link-to-dir-1"), &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(LnTests, tebako_lstat_absolute_path_no_file) {
		struct stat st;
		int ret = tebako_stat(TEBAKIZE_PATH("no_file"), &st);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(LnTests, tebako_lstat_null) {
		struct stat st;
		int ret = tebako_lstat(NULL, &st);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(LnTests, tebako_lstat_relative_path) {
		struct stat st;
		int ret = tebako_chdir(TEBAKIZE_PATH(""));
		EXPECT_EQ(0, ret);
		ret = tebako_stat("s-link-to-dir-1", &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(LnTests, tebako_lstat_relative_path_no_file) {
		struct stat st;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_lstat("no_file", &st);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(LnTests, tebako_lstat_absolute_path_pass_through) {
		struct stat st;
		int ret = tebako_lstat("/usr/bin/sh", &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(LnTests, tebako_lstat_relative_path_pass_through) {
		struct stat st;
		int ret = tebako_chdir("/usr/bin");
		EXPECT_EQ(0, ret);
		ret = tebako_lstat("sh", &st);
		EXPECT_EQ(0, ret);
	}
#endif

}
