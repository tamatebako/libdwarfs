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
		const int num2read = 4;

		int ret = tebako_read(fh, readbuf, num2read); readbuf[num2read] = '\0';
		EXPECT_EQ(num2read, ret);
		EXPECT_EQ(0, strcmp(readbuf, "This"));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_lseek_read_close_absolute_path) {
		int fh = tebako_open(2, TEBAKIZE_PATH("directory-1/file-in-directory-1.txt"), O_RDONLY);
		EXPECT_LT(0, fh);

		const char* pattern = "This is a file in directory";
		const int l = strlen(pattern);
		const int off2move = 10;

		int ret = tebako_lseek(fh, off2move, SEEK_SET);
		EXPECT_EQ(off2move, ret);

		ret = tebako_lseek(fh, 0, SEEK_END);
		EXPECT_EQ(l, ret);

		ret = tebako_lseek(fh, -off2move, SEEK_CUR);
		EXPECT_EQ(l-off2move, ret);


		char readbuf[32];
		const int num2read = 4;
		ret = tebako_read(fh, readbuf, num2read); readbuf[num2read] = '\0';
		EXPECT_EQ(num2read, ret);
		EXPECT_EQ(0, strncmp(readbuf, pattern+l-off2move, num2read));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_lseek_readv_close_absolute_path) {
		int fh = tebako_open(2, TEBAKIZE_PATH("directory-2/file-in-directory-2.txt"), O_RDONLY);
		EXPECT_LT(0, fh);

		const char* pattern = "This is a file in a second directory";
		const int l = strlen(pattern);
		const int off2move = 10;

		int ret = tebako_lseek(fh, off2move, SEEK_SET);
		EXPECT_EQ(10, ret);

		ret = tebako_lseek(fh, 0, SEEK_END);
		EXPECT_EQ(l, ret);

		ret = tebako_lseek(fh, -off2move, SEEK_CUR);
		EXPECT_EQ(l-off2move, ret);

		const int bsize = off2move/2+1;
		char buf0[bsize];
		char buf1[bsize];
		char buf2[bsize];
		int iovcnt;
		struct iovec iov[3];


		iov[0].iov_base = buf0;
		iov[0].iov_len = sizeof(buf0);
		iov[1].iov_base = buf1;
		iov[1].iov_len = sizeof(buf1);
		iov[2].iov_base = buf2;
		iov[2].iov_len = sizeof(buf2);
		iovcnt = sizeof(iov) / sizeof(struct iovec);

		ret = tebako_readv(fh, &iov[0], iovcnt);

		EXPECT_EQ(off2move, ret);
		EXPECT_EQ(0, strncmp(buf0, pattern + l - off2move, bsize));
		EXPECT_EQ(0, strncmp(buf1, pattern + l - off2move+bsize, bsize-2));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_read_u_close_relative_path) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);

		int fh = tebako_open(2, "file-in-directory-2.txt" , O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[64];
		const char* pattern = "This is a file in a second directory";
		const int l = strlen(pattern);
		ret = tebako_read(fh, readbuf, sizeof(readbuf)/sizeof(readbuf[0]));
		EXPECT_EQ(l, ret);
		EXPECT_EQ(0, strncmp(readbuf, pattern, l));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_pread_u_close_relative_path) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);

		int fh = tebako_open(2, "file-in-directory-2.txt", O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[64];
		const char* pattern = "This is a file in a second directory";
		const int l = strlen(pattern);
		const int offset = 10;
		ret = tebako_pread(fh, readbuf, sizeof(readbuf) / sizeof(readbuf[0]), offset);
		EXPECT_EQ(l - offset, ret);
		EXPECT_EQ(0, strncmp(readbuf, pattern + offset, l - offset));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_pread_invalid_handle) {
		int ret = tebako_chdir(TEBAKIZE_PATH("directory-2"));
		EXPECT_EQ(0, ret);

		char readbuf[64];
		const char* pattern = "This is a file in a second directory";
		const int l = strlen(pattern);
		const int offset = 10;
		ret = tebako_pread(33, readbuf, sizeof(readbuf) / sizeof(readbuf[0]), offset);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EBADF, errno);
	}

	TEST_F(FileIOTests, tebako_openat_invalid_handle) {
		int ret = tebako_openat(3, 33, "file.txt", O_RDONLY);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EBADF, errno);
	}

	TEST_F(FileIOTests, tebako_openat_rdwr) {
		int fh1 = tebako_open(2, TEBAKIZE_PATH("directory-1"), O_RDONLY);
		EXPECT_LT(0, fh1);
		int fh2 = tebako_openat(3, fh1, "file2-in-directory-2.txt", O_RDWR);
		EXPECT_EQ(-1, fh2);
		EXPECT_EQ(EROFS, errno);
		EXPECT_EQ(0, tebako_close(fh1));
	}

	TEST_F(FileIOTests, tebako_openat_not_relative) {
		int fh1 = tebako_open(2, TEBAKIZE_PATH("directory-1"), O_RDONLY);
		EXPECT_LT(0, fh1);
		int fh2 = tebako_openat(3, fh1, TEBAKIZE_PATH("file.txt"), O_RDONLY);
		EXPECT_LT(0, fh2);

		char readbuf[32];
		const char* pattern = "Just a file";
		const int num2read = strlen(pattern);
		EXPECT_EQ(num2read, tebako_read(fh2, readbuf, sizeof(readbuf)/sizeof(readbuf[0]))); 
		EXPECT_EQ(0, strncmp(readbuf, "Just a file", num2read));

		EXPECT_EQ(0, tebako_close(fh1));
		EXPECT_EQ(0, tebako_close(fh2));
	}

	TEST_F(FileIOTests, tebako_openat_atcwd) {
		int fh1 = tebako_open(2, TEBAKIZE_PATH("directory-1"), O_RDONLY);
		EXPECT_LT(0, fh1);

		EXPECT_EQ(0, tebako_chdir(TEBAKIZE_PATH("")));
		int fh2 = tebako_openat(3, AT_FDCWD, TEBAKIZE_PATH("file.txt"), O_RDONLY);
		EXPECT_LT(0, fh2);

		char readbuf[32];
		const char* pattern = "Just a file";
		const int num2read = strlen(pattern);
		EXPECT_EQ(num2read, tebako_read(fh2, readbuf, sizeof(readbuf) / sizeof(readbuf[0])));
		EXPECT_EQ(0, strncmp(readbuf, "Just a file", num2read));

		EXPECT_EQ(0, tebako_close(fh1));
		EXPECT_EQ(0, tebako_close(fh2));
	}

	TEST_F(FileIOTests, tebako_open_lseek_read_close_absolute_path_pass_through) {
		int fh1 = tebako_open(2, "/bin/sh", O_RDONLY);
		EXPECT_LT(0, fh1);
		int ret = tebako_lseek(fh1, 5, SEEK_SET);
		EXPECT_EQ(5, ret);
		char readbuf[64];
		ret = tebako_read(fh1, readbuf, sizeof(readbuf) / sizeof(readbuf[0]));
		EXPECT_EQ(sizeof(readbuf) / sizeof(readbuf[0]), ret);

		ret = tebako_close(fh1);
		EXPECT_EQ(0, ret);
	}

	TEST_F(FileIOTests, tebako_open_openat_close_interop) {
		int fh1 = ::open("/bin", O_RDONLY|O_DIRECTORY);
		EXPECT_LT(0, fh1);

		int fh2 = tebako_openat(3, fh1, "bash", O_RDONLY);
		EXPECT_LT(0, fh2);

		EXPECT_EQ(0, ::close(fh1));
		EXPECT_EQ(0, tebako_close(fh2));

		fh1 = tebako_open(2, "/bin", O_RDONLY);
		EXPECT_LT(0, fh1);

		fh2 = tebako_openat(3, fh1, "bash", O_RDONLY);
		EXPECT_LT(0, fh2);

		EXPECT_EQ(0, close(fh1));
		EXPECT_EQ(0, tebako_close(fh2));
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