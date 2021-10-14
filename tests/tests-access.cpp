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
 *  Unit tests for 'tebako_access' function and underlying 'dwarfs_access'
 */

namespace {
	class AccessTests : public testing::Test {
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

	TEST_F(AccessTests, tebako_access_absolute_path) {
		int ret = tebako_access(TEBAKIZE_PATH("file.txt"), F_OK);
		EXPECT_EQ(0, ret);
	}

	TEST_F(AccessTests, tebako_access_absolute_path_no_file) {
		int ret = tebako_access(TEBAKIZE_PATH("no-directory/file.txt"), W_OK);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(AccessTests, tebako_access_relative_path) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_access("file-in-directory-2.txt", R_OK);
		EXPECT_EQ(0, ret);
	}

	TEST_F(AccessTests, tebako_access_relative_path_no_file) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);
		ret = tebako_access("no-file-in-directory-2.txt", R_OK);
		EXPECT_EQ(ENOENT, errno);
		EXPECT_EQ(-1, ret);
	}

	TEST_F(AccessTests, tebako_access_absolute_path_pass_through) {
		int ret = tebako_access("/usr/bin/bash", F_OK);
		EXPECT_EQ(0, ret);
	}

	TEST_F(AccessTests, tebako_access_relative_path_pass_through) {
		int ret = tebako_chdir("/usr/");
		EXPECT_EQ(0, ret);
		ret = tebako_access("bin", R_OK|X_OK);
		EXPECT_EQ(0, ret);
	}

	TEST_F(AccessTests, tebako_access_absolute_path_no_access) {
		int ret = tebako_access(TEBAKIZE_PATH("restricted-do-not-touch.txt"), W_OK);
		EXPECT_EQ(EACCES, errno);
		EXPECT_EQ(-1, ret);
	}
}
