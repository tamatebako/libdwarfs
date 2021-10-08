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
*  Unit tests for 'tebako_open', 'tebako_close', 'tebako_read', 'tebako_write'
* 'tebako_lseek' and underlying file descriptor implementation
*/


namespace {
	class FileIOTests : public testing::Test {
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

	TEST_F(FileIOTests, tebako_open_rdwr) {
		int ret = tebako_open(2, TEBAKIZE_PATH("file.txt"), O_RDWR);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EROFS, errno);
	}

	TEST_F(FileIOTests, tebako_open_wronly) {
		int ret = tebako_chdir(TEBAKIZE_PATH(""));
		EXPECT_EQ(0, ret);
		ret = tebako_open(2, "file.txt", O_WRONLY);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EROFS, errno);
	}

	TEST_F(FileIOTests, tebako_open_create) {
		int ret = tebako_open(2, TEBAKIZE_PATH("no-file.txt"), O_RDONLY|O_CREAT);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EROFS, errno);
	}

	TEST_F(FileIOTests, tebako_open_truncate) {
		int ret = tebako_open(2, TEBAKIZE_PATH("file.txt"), O_RDONLY|O_TRUNC);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EROFS, errno);
	}

	TEST_F(FileIOTests, tebako_open_no_file) {
		int ret = tebako_open(2, TEBAKIZE_PATH("no-file.txt"), O_RDONLY);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(ENOENT, errno);
	}

	TEST_F(FileIOTests, tebako_fio_absolute_path_pass_through) {
		int ret = tebako_open(2, "/bin/sh", O_RDONLY);
		EXPECT_LT(0, ret);
		ret = tebako_close(ret);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_fio_relative_path_pass_through) {
		int ret = tebako_chdir("/bin");
		EXPECT_EQ(0, ret);
		ret = tebako_open(2, "sh", O_RDONLY);
		EXPECT_LT(0, ret);
		ret = tebako_close(ret);
		EXPECT_EQ(0, ret);
	}

}
