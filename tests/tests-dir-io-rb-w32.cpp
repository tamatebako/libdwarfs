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
 */

#include "tests.h"
#include <tebako-io-rb-w32-inner.h>

namespace {
	class DirIORbW32Tests : public testing::Test {
	protected:
		static void SetUpTestSuite() {
			do_rb_w32_init();
			load_fs(&gfsData[0],
				gfsSize,
				tests_log_level,
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

	TEST_F(DirIORbW32Tests, tebako_opendir_no_dir) {
		DIR* dirp = tebako_opendir(TEBAKIZE_PATH("no_directory"));
		EXPECT_EQ(NULL, dirp);
		EXPECT_EQ(ENOENT, errno);
	}

	TEST_F(DirIORbW32Tests, tebako_opendir_not_dir) {
		DIR* dirp = tebako_opendir(TEBAKIZE_PATH("file.txt"));
		EXPECT_EQ(NULL, dirp);
		EXPECT_EQ(ENOTDIR, errno);
	}



	TEST_F(DirIORbW32Tests, tebako_opendir_seekdir_telldir_readdir_closedir) {
		DIR* dirp = tebako_opendir(TEBAKIZE_PATH("directory-with-90-files"));
		EXPECT_TRUE(dirp != NULL);
		if (dirp != NULL) {
			errno = 0;

			const off_t start_fnum = 10;  /* The first file nane is file - 10.txt */
			const size_t size_dir = 89;
			long pos = 49;
			std::string fname;

			tebako_seekdir(dirp, pos);
			EXPECT_EQ(pos, tebako_telldir(dirp));

			struct direct* entry = tebako_readdir(dirp, 0);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "file-"; fname += std::to_string(pos + start_fnum - 2 /* for '.' and '..' */); fname += ".txt";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_TRUE(entry->d_type == DT_REG);
				EXPECT_EQ(++pos, tebako_telldir(dirp));
			}

			entry = tebako_readdir(dirp, 0);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "file-"; fname += std::to_string(pos + start_fnum - 2 /* for '.' and '..' */); fname += ".txt";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_EQ(++pos, tebako_telldir(dirp));
			}

			entry = tebako_readdir(dirp, 0);                       // Expecting direct buffer reload at this point
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "file-"; fname += std::to_string(pos + start_fnum - 2 /* for '.' and '..' */); fname += ".txt";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_EQ(++pos, tebako_telldir(dirp));
			}

			pos = size_dir + 2; /* for '.' and '..' */
			tebako_seekdir(dirp, pos);
			EXPECT_EQ(pos, tebako_telldir(dirp));

			entry = tebako_readdir(dirp, 0);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "file-"; fname += std::to_string(pos + start_fnum - 2 /* for '.' and '..' */); fname += ".txt";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_EQ(++pos, tebako_telldir(dirp));
			}

			entry = tebako_readdir(dirp, 0);					// Expecting read beyond dir size
			EXPECT_TRUE(entry == NULL);

			tebako_seekdir(dirp, 0);
			EXPECT_EQ(0, tebako_telldir(dirp));
			entry = tebako_readdir(dirp, 0);					// Expecting direct buffer reload at this point
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = ".";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_TRUE(entry->d_type == DT_DIR);
				EXPECT_EQ(1, tebako_telldir(dirp));
			}

			EXPECT_EQ(0, tebako_closedir(dirp));
		}
	}

	TEST_F(DirIORbW32Tests, tebako_opendir_seekdir_telldir_readdir_closedir_pass_through) {
		const char * const shell_name =	"bash.exe";
		const char * const shell_folder = __MSYS_BIN__;

		DIR* dirp = tebako_opendir(shell_folder);
		EXPECT_TRUE(dirp != NULL);
		if (dirp != NULL) {
			long loc = -1;
			errno = 0;
			struct direct* entry = tebako_readdir(dirp, 0);
			while (entry != NULL) {
				long l = telldir(dirp);
				entry = tebako_readdir(dirp, 0);
				if (entry != NULL && strcmp(entry->d_name, shell_name) == 0) {
					loc = l;
				}
			}
			EXPECT_NE(-1, loc);
			EXPECT_EQ(errno, 0);

            if (loc !=-1) {
			  tebako_seekdir(dirp, loc);
			  entry = tebako_readdir(dirp, 0);
			  EXPECT_TRUE(strcmp(entry->d_name, shell_name)==0);
			}

			EXPECT_EQ(0, tebako_closedir(dirp));
		}
	}

	TEST_F(DirIORbW32Tests, tebako_dir_io_null_ptr) {
		errno = 0;
		EXPECT_EQ(NULL, tebako_opendir(NULL));
		EXPECT_EQ(ENOENT, errno);

		errno = 0;
		EXPECT_EQ(-1, tebako_telldir(NULL));
		EXPECT_EQ(EBADF, errno);

		tebako_seekdir(NULL, 1);   // Just nothing. No error, no SEGFAULT

		errno = 0;
		EXPECT_EQ(-1, tebako_closedir(NULL));
		EXPECT_EQ(EBADF, errno);

		errno = 0;
		EXPECT_EQ(NULL, tebako_readdir(NULL, 0));
		EXPECT_EQ(EBADF, errno);
	}

	TEST_F(DirIORbW32Tests, tebako_opendir_readdir_closedir_dot_dot) {
		DIR* dirp = tebako_opendir(TEBAKIZE_PATH("directory-3/level-1/.//level-2/level-3/.."));
		EXPECT_TRUE(dirp != NULL);
		if (dirp != NULL) {
			std::string fname;
			std::string fname_alt;
			struct direct* entry = tebako_readdir(dirp, 0);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = ".";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_TRUE(entry->d_type == DT_DIR);
			}
			entry = tebako_readdir(dirp, 0);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "..";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_TRUE(entry->d_type == DT_DIR);
			}
			fname = "test-file-at-level-2.txt";
			fname_alt = "level-3";
			entry = tebako_readdir(dirp, 0);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				EXPECT_TRUE((fname == entry->d_name) || (fname_alt == entry->d_name));
				EXPECT_TRUE(entry->d_type == (entry->d_name == fname_alt ? DT_DIR : DT_REG));
			}
			entry = tebako_readdir(dirp, 0);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				EXPECT_TRUE((fname == entry->d_name) || (fname_alt == entry->d_name));
				EXPECT_TRUE(entry->d_type == (entry->d_name == fname_alt ? DT_DIR : DT_REG));
			}
			EXPECT_EQ(0, tebako_closedir(dirp));
		}
	}
}
