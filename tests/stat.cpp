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

/*  
*  Unit tests for 'tebako_stat' function and underlying 'dwarfs_stat'
*/


namespace {
	class StatTests : public testing::Test {
	protected:
		static void SetUpTestSuite() {
			load_fs(&gfsData[0],
				gfsSize,
				"debug" /*debuglevel*/,
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

	TEST_F(StatTests, tebako_stat_absolute_path) {
		struct stat buf;
		int ret = tebako_stat(TEBAKIZE_PATH("file.txt"), &buf);
		EXPECT_EQ(0, ret);
	}

	TEST_F(StatTests, tebako_stat_absolute_path_no_file) {
		struct stat buf;
		int ret = tebako_stat(TEBAKIZE_PATH("no_file.txt"), &buf);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(StatTests, tebako_stat_relative_path) {
		struct stat buf;
		int ret = tebako_chdir(TEBAKIZE_PATH(""));
		EXPECT_EQ(0, ret);
		ret = tebako_stat("directory-1/file-in-directory-1.txt", &buf);
		EXPECT_EQ(0, ret);
	}

	TEST_F(StatTests, tebako_stat_relative_path_no_file) {
		struct stat buf;
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_stat("no_file.txt", &buf);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(StatTests, tebako_stat_absolute_path_pass_through) {
		struct stat buf;
		int ret = tebako_stat("/usr/bin/bash", &buf);
		EXPECT_EQ(0, ret);
	}

	TEST_F(StatTests, tebako_stat_relative_path_pass_through) {
		struct stat buf;
		int ret = tebako_chdir("/usr/bin");
		EXPECT_EQ(0, ret);
		ret = tebako_stat("sh", &buf);
		EXPECT_EQ(0, ret);
	}
}
