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
*  Unit tests for 'tebako_open', 'tebako_close', 'tebako_read'
* 'tebako_lseek' and underlying file descriptor implementation
*/


namespace {
	class FileIOTests : public testing::Test {
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

	TEST_F(FileIOTests, tebako_open_dir) {
		int ret = tebako_open(2, TEBAKIZE_PATH("directory-1"), O_RDONLY|O_DIRECTORY);
		EXPECT_LT(0, ret);
		ret = tebako_close(ret);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_not_dir) {
		int ret = tebako_open(2, TEBAKIZE_PATH("directory-1/file-in-directory-1.txt"), O_RDONLY | O_DIRECTORY);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(ENOTDIR, errno);
	}

	TEST_F(FileIOTests, tebako_close_bad_file) {
		int ret = tebako_close(33);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EBADF, errno);
	}

	TEST_F(FileIOTests, tebako_open_read_close_absolute_path) {
		int fh = tebako_open(2, TEBAKIZE_PATH("directory-1/file-in-directory-1.txt"), O_RDONLY );
		EXPECT_LT(0, fh);

		char readbuf[32];
		int ret = tebako_read(fh, readbuf, 4); readbuf[4] = '\0';
		EXPECT_EQ(4, ret);
		EXPECT_EQ(0, strcmp(readbuf, "This"));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_lseek_read_close_absolute_path) {
		int fh = tebako_open(2, TEBAKIZE_PATH("directory-1/file-in-directory-1.txt"), O_RDONLY);
		EXPECT_LT(0, fh);

		const char* pattern = "This is a file in directory";
		const int l = strlen(pattern);

		int ret = tebako_lseek(fh, 10, SEEK_SET);
		EXPECT_EQ(10, ret);

		ret = tebako_lseek(fh, 0, SEEK_END);
		EXPECT_EQ(l, ret);

		ret = tebako_lseek(fh, -10, SEEK_CUR);
		EXPECT_EQ(l-10 , ret);


		char readbuf[32];
		ret = tebako_read(fh, readbuf, 4); readbuf[4] = '\0';
		EXPECT_EQ(4, ret);
		EXPECT_EQ(0, strncmp(readbuf, pattern+l-10, 4));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_read_u_close_relative_path) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);

		int fh = tebako_open(2, "file-in-directory-2.txt" , O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[32];
		const char* pattern = "This is a file in directory";
		const int l = strlen(pattern);
		ret = tebako_read(fh, readbuf, 32); 
		EXPECT_EQ(l, ret);
		EXPECT_EQ(0, strncmp(readbuf, pattern, l));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_close_absolute_path_pass_through) {
		int ret = tebako_open(2, "/bin/sh", O_RDONLY);
		EXPECT_LT(0, ret);
		ret = tebako_close(ret);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_close_relative_path_pass_through) {
		int ret = tebako_chdir("/bin");
		EXPECT_EQ(0, ret);
		ret = tebako_open(2, "sh", O_RDONLY);
		EXPECT_LT(0, ret);
		ret = tebako_close(ret);
		EXPECT_EQ(0, ret);
	}

}
