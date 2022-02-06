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
 **/

#include "tests.h"

 /**
 *  - Unit tests for 'load_fs/drop_fs' functions
 *  - Unit tests for tebako_xxx functions for the case when dwarfs is not loaded
 *    (placed here since all other test units contain fixture that loads dwarfs)
 **/

namespace {
	TEST(LoadTests, smoke) {
		int i = 1;
		EXPECT_EQ(1, i);
	}

//ifndef WITH_ASAN
// ASAN cannot survive DWARFS_THROW [??] 

	TEST(LoadTests, tebako_load_invalid_filesystem) {
		const unsigned char data[] = "This is broken filesystem image";
		int ret = load_fs(	&data[0],
							sizeof(data)/sizeof(data[0]),
							tests_log_level,
							NULL	/* cachesize*/,
							NULL	/* workers */,
							NULL	/* mlock */,
							NULL	/* decompress_ratio*/,
							NULL    /* image_offset */
			);
		EXPECT_EQ(-1, ret);
		drop_fs();
	}

	TEST(LoadTests, tebako_load_invalid_parameter) {
		int ret = load_fs(&gfsData[0],
			gfsSize,
			"invalid parameter" /*debuglevel*/,
			NULL	/* cachesize*/,
			NULL	/* workers */,
			NULL	/* mlock */,
			NULL	/* decompress_ratio*/,
			NULL    /* image_offset */
		);

		EXPECT_EQ(1, ret);
		drop_fs();
	}

	TEST(LoadTests, tebako_load_valid_filesystem) {
		int ret = load_fs(	&gfsData[0],
							gfsSize,
							tests_log_level,
							NULL	/* cachesize*/,
							NULL	/* workers */,
							NULL	/* mlock */,
							NULL	/* decompress_ratio*/,
							NULL    /* image_offset */
		);

		EXPECT_EQ(0, ret);
		drop_fs();
	}

	TEST(LoadTests, tebako_stat_not_loaded_filesystem) {
		struct stat buf;
		int ret = tebako_stat(TEBAKIZE_PATH("file.txt"), &buf);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(ENOENT, errno);
	}

	TEST(LoadTests, tebako_access_not_loaded_filesystem) {
		struct stat buf;
		int ret = tebako_access(TEBAKIZE_PATH("file.txt"), W_OK);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(ENOENT, errno);
	}

	TEST(LoadTests, tebako_open_not_loaded_filesystem) {
		struct stat buf;
		int ret = tebako_open(2, TEBAKIZE_PATH("file.txt"), O_RDONLY);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(ENOENT, errno);
	}

	TEST(LoadTests, tebako_close_all_fd) {
		int ret = load_fs(&gfsData[0],
			gfsSize,
			tests_log_level,
			NULL	/* cachesize*/,
			NULL	/* workers */,
			NULL	/* mlock */,
			NULL	/* decompress_ratio*/,
			NULL    /* image_offset */
		);

		EXPECT_EQ(0, ret);
		int fh = tebako_open(2, TEBAKIZE_PATH("file.txt"), O_RDONLY);
		EXPECT_LT(0, fh);
		drop_fs();

		char readbuf[32];
		const int num2read = 4;

		ret = tebako_read(fh, readbuf, num2read);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EBADF, errno);
	}

	TEST(LoadTests, tebako_close_all_dir) {
		int ret = load_fs(&gfsData[0],
			gfsSize,
			tests_log_level,
			NULL	/* cachesize*/,
			NULL	/* workers */,
			NULL	/* mlock */,
			NULL	/* decompress_ratio*/,
			NULL    /* image_offset */
		);

		EXPECT_EQ(0, ret);
		DIR * dirp = tebako_opendir(TEBAKIZE_PATH("directory-1"));
		EXPECT_TRUE(dirp!=NULL);
		drop_fs();

/*
*	If the end of the directory stream is reached, NULL is returned
*	and errno is not changed.If an error occurs, NULL is returned
*	and errno is set to indicate the error.To distinguish end of
*	stream from an error, set errno to zero before calling readdir()
*	and then check the value of errno if NULL is returned.
*/

		tebako_seekdir(dirp, 3);

		errno = 0;
		long loc = tebako_telldir(dirp);
		EXPECT_EQ(-1L, loc);
		EXPECT_EQ(EBADF, errno);


		errno = 0;
		struct dirent* entry = tebako_readdir(dirp);
		EXPECT_EQ(NULL, entry);
		EXPECT_EQ(EBADF, errno);

		tebako_rewinddir(dirp);

		errno = 0;
		ret = tebako_closedir(dirp);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EBADF, errno);
	}
}
