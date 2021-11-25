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
	class DirIOTests : public testing::Test {
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

	TEST_F(DirIOTests, tebako_opendir_no_dir) {
		DIR* dirp = tebako_opendir(TEBAKIZE_PATH("no_directory"));
		EXPECT_EQ(NULL, dirp);
		EXPECT_EQ(ENOENT, errno);
	}

	TEST_F(DirIOTests, tebako_opendir_not_dir) {
		DIR* dirp = tebako_opendir(TEBAKIZE_PATH("file.txt"));
		EXPECT_EQ(NULL, dirp);
		EXPECT_EQ(ENOTDIR, errno);
	}

	TEST_F(DirIOTests, tebako_fdopendir_not_dir) {
		int fh = tebako_open(2, TEBAKIZE_PATH("file.txt"), O_RDONLY);
		EXPECT_LT(0, fh);
		DIR* dirp = tebako_fdopendir(fh);
		EXPECT_EQ(NULL, dirp);
		EXPECT_EQ(ENOTDIR, errno);
		EXPECT_EQ(0, tebako_close(fh));
	}

	TEST_F(DirIOTests, tebako_fdopendir_not_invalid_handle) {
		DIR* dirp = tebako_fdopendir(33);
		EXPECT_EQ(NULL, dirp);
		EXPECT_EQ(EBADF, errno);
	}

	TEST_F(DirIOTests, tebako_fdopendir_dirfd_closedir) {
		int fh = tebako_open(2, TEBAKIZE_PATH("directory-1"), O_RDONLY);
		EXPECT_LT(0, fh);
		DIR* dirp = tebako_fdopendir(fh);
		EXPECT_TRUE(dirp != NULL);
		EXPECT_EQ(fh, tebako_dirfd(dirp));
		EXPECT_EQ(0, tebako_closedir(dirp));

	}

	TEST_F(DirIOTests, tebako_opendir_seekdir_telldir_readdir_rewinddir_closedir) {
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

			struct dirent* entry = tebako_readdir(dirp);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "file-"; fname += std::to_string(pos + start_fnum - 2 /* for '.' and '..' */); fname += ".txt";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_EQ(++pos, tebako_telldir(dirp));
			}

			entry = tebako_readdir(dirp);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "file-"; fname += std::to_string(pos + start_fnum - 2 /* for '.' and '..' */); fname += ".txt";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_EQ(++pos, tebako_telldir(dirp));
			}

			entry = tebako_readdir(dirp);                       // Expecting dirent buffer reload at this point
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "file-"; fname += std::to_string(pos + start_fnum - 2 /* for '.' and '..' */); fname += ".txt";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_EQ(++pos, tebako_telldir(dirp));
			}

			pos = size_dir + 2; /* for '.' and '..' */
			tebako_seekdir(dirp, pos);
			EXPECT_EQ(pos, tebako_telldir(dirp));

			entry = tebako_readdir(dirp);
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = "file-"; fname += std::to_string(pos + start_fnum - 2 /* for '.' and '..' */); fname += ".txt";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_EQ(++pos, tebako_telldir(dirp));
			}

			entry = tebako_readdir(dirp);					// Expecting read beyond dir size
			EXPECT_TRUE(entry == NULL);

			tebako_rewinddir(dirp);
			EXPECT_EQ(0, tebako_telldir(dirp));
			entry = tebako_readdir(dirp);					// Expecting dirent buffer reload at this point
			EXPECT_TRUE(entry != NULL);
			if (entry != NULL) {
				fname = ".";
				EXPECT_TRUE(fname == entry->d_name);
				EXPECT_EQ(1, tebako_telldir(dirp));
			}

			EXPECT_EQ(0, tebako_closedir(dirp));
		}
	}

	TEST_F(DirIOTests, tebako_opendir_seekdir_telldir_readdir_rewinddir_closedir_pass_through) {
		DIR* dirp = tebako_opendir("/bin");
		EXPECT_TRUE(dirp != NULL);
		if (dirp != NULL) {
			long loc = -1;
			errno = 0;
			struct dirent* entry = tebako_readdir(dirp);
			while (entry != NULL) {
				entry = tebako_readdir(dirp);
				if (entry != NULL && strcmp(entry->d_name, "bash") == 0) {
					loc = telldir(dirp) - 1;
				}
			}
			EXPECT_NE(-1, loc);
			EXPECT_EQ(errno, 0);

			tebako_seekdir(dirp, loc);
			entry = tebako_readdir(dirp);
			EXPECT_TRUE(strcmp(entry->d_name, "bash"));

			EXPECT_EQ(0, tebako_closedir(dirp));
		}
	}

	TEST_F(DirIOTests, tebako_scandir) {
		const off_t start_fnum = 10;  /* The first filename is 'file-10.txt' */
		const size_t size_dir = 90;
		struct dirent** namelist;
		std::string fname;
		int n = tebako_scandir(TEBAKIZE_PATH("directory-with-90-files"), &namelist, NULL, alphasort);
		EXPECT_EQ(n, size_dir + 2);
		EXPECT_TRUE(namelist != NULL);
		if (n > 0 && namelist != NULL) {
			for (int i = 0; i < n; i++) {
				switch (i) {
				case 0:
					fname = ".";
					break;
				case 1:
					fname = "..";
					break;
				default:
					fname = "file-";
					fname += std::to_string(i + start_fnum - 2 /* for '.' and '..' */);
					fname += ".txt";
					break;
				}
				EXPECT_TRUE(namelist[i] != NULL);
				if (namelist[i]) {
					EXPECT_TRUE(fname == namelist[i]->d_name);
					free(namelist[i]);
				}
			}
			free(namelist);
		}
	}

	extern "C" int zero_filter(const struct dirent* )
	{
		return 0;
	}

	TEST_F(DirIOTests, tebako_scandir_filter_empty) {
		struct dirent** namelist;
		std::string fname;
		int n = tebako_scandir(TEBAKIZE_PATH("directory-with-90-files"), &namelist, zero_filter, NULL);
		EXPECT_EQ(0, n);
		EXPECT_TRUE(namelist != NULL);
		if (namelist != NULL) {
			free(namelist);
		}
	}

	extern "C" int bash_filter(const struct dirent* entry)
	{
		return (strcmp(entry->d_name, "bash") == 0);
	}

	TEST_F(DirIOTests, tebako_scandir_filter_pass_through) {
		struct dirent** namelist;
		std::string fname;
		int n = tebako_scandir("/bin", &namelist, bash_filter, NULL);
		EXPECT_EQ(1, n);
		EXPECT_TRUE(namelist != NULL);
		if (namelist != NULL) {
			EXPECT_TRUE(strcmp(namelist[0]->d_name, "bash") == 0);
			free(namelist[0]);
			free(namelist);
		}
	}


	TEST_F(DirIOTests, tebako_dir_io_null_ptr) {
		errno = 0;
		EXPECT_EQ(NULL, tebako_opendir(NULL));
		EXPECT_EQ(ENOENT, errno);

		errno = 0;
		EXPECT_EQ(-1, tebako_telldir(NULL));
		EXPECT_EQ(EBADF, errno);

		tebako_seekdir(NULL, 1);   // Just nothing. No error, no SEGFAULT
		tebako_rewinddir(NULL);    // The same

		errno = 0;
		EXPECT_EQ(-1, tebako_dirfd(NULL));
		EXPECT_EQ(EBADF, errno);

		errno = 0;
		EXPECT_EQ(-1, tebako_closedir(NULL));
		EXPECT_EQ(EBADF, errno);

		errno = 0;
		EXPECT_EQ(NULL, tebako_readdir(NULL));
		EXPECT_EQ(EBADF, errno);


		struct dirent** namelist;

		errno = 0;
		EXPECT_EQ(-1, tebako_scandir(NULL, &namelist, zero_filter, alphasort));
		EXPECT_EQ(ENOENT, errno);

		errno = 0;
		EXPECT_EQ(-1, tebako_scandir("somedir", NULL, zero_filter, alphasort));
		EXPECT_EQ(ENOENT, errno);

	}
}
