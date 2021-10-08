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

	TEST(LoadTests, tebako_load_invalid_filesystem) {
		const unsigned char data[] = "This is broken filesystem image";
		int ret = load_fs(	&data[0], 
							sizeof(data)/sizeof(data[0]),
							"debug" /*debuglevel*/,
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
							"debug" /*debuglevel*/,
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
}
