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
#include <filesystem>

namespace fs = std::filesystem;

namespace {
	class LnTests : public testing::Test {
	protected:
		static fs::path tmp_path;
		static bool cross_test;
		static bool path_initialized;
		static void SetUpTestSuite() {
			load_fs(&gfsData[0],
				gfsSize,
				tests_log_level,
				NULL	/* cachesize*/,
				NULL	/* workers */,
				NULL	/* mlock */,
				NULL	/* decompress_ratio*/,
				NULL    /* image_offset */
			);

			std::string tdp_template = (fs::temp_directory_path() / "libdwarfs.tests.XXXXXX");
			size_t l = tdp_template.length();
			char* dir_name = new char[l+1];
			if (dir_name) {
				strcpy(dir_name, tdp_template.c_str());
				dir_name = mkdtemp(dir_name);
				tmp_path = dir_name;
				fs::create_directories(tmp_path);
				path_initialized = true;

				fs::create_symlink("/bin/true", tmp_path / "link2true");
				fs::create_symlink("/bin/false", tmp_path / "link2false");

				delete[] dir_name;
			}

			cross_test = (std::getenv("TEBAKO_CROSS_TEST") != NULL);
		}

		static void TearDownTestSuite() {
			if (path_initialized) {
				fs::remove_all(tmp_path);
			}
			path_initialized = false;
			drop_fs();
		}

	};
	bool LnTests::cross_test = false;
	bool LnTests::path_initialized = false;
	fs::path LnTests::tmp_path;

#ifdef WITH_LINK_TESTS
	TEST_F(LnTests, tebako_softlink) {
		int fh = tebako_open(2, TEBAKIZE_PATH("s-link-to-file-1"), O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);

		int ret = tebako_read(fh, readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_EQ(0, strncmp(readbuf, "This is a file in the first directory", num2read));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

	TEST_F(LnTests, tebako_softlink_not_file) {
		int fh = tebako_open(2, TEBAKIZE_PATH("s-link-to-dir-1"), O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);

		int ret = tebako_read(fh, readbuf, num2read);
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EBADF, errno);

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}

/*
	TEST_F(LnTests, tebako_softlink_to_dir_open) {
		int fh = tebako_open(2, TEBAKIZE_PATH("s-link-to-dir-1/file-in-directory-2.txt"), O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);

		int ret = tebako_read(fh, readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_EQ(0, strncmp(readbuf, "This is a file in the second directory", num2read));

		ret = tebako_close(fh);
		EXPECT_EQ(0, ret);
	}
*/

	TEST_F(LnTests, tebako_hardlink) {
		int fh = tebako_open(2, TEBAKIZE_PATH("h-link-to-file-2"), O_RDONLY);
		EXPECT_LT(0, fh);

		char readbuf[32];
		const int num2read = sizeof(readbuf)/sizeof(readbuf[0]);

		int ret = tebako_read(fh, readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_TRUE(strncmp(readbuf, "This is a file in the second directory", num2read) == 0);

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
		int ret = tebako_readlink(TEBAKIZE_PATH("s-link-to-file-1"), readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_TRUE(strncmp(readbuf, "directory-1/file-in-directory-1.txt", num2read) == 0);
	}

	TEST_F(LnTests, tebako_readlink_relative_path) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		EXPECT_EQ(0, tebako_chdir(TEBAKIZE_PATH("")));
		int ret = tebako_readlink("s-link-to-file-1", readbuf, num2read);
		EXPECT_EQ(num2read, ret);
		EXPECT_TRUE(strncmp(readbuf, "directory-1/file-in-directory-1.txt", num2read) == 0);
	}

	TEST_F(LnTests, tebako_readlink_absolute_path_pass_through) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		int ret = tebako_readlink((tmp_path / "link2false").c_str(), readbuf, num2read);
		EXPECT_EQ(strlen("/bin/false"), ret);
		EXPECT_TRUE(strncmp(readbuf, "/bin/false", ret) == 0);
	}

	TEST_F(LnTests, tebako_readlink_relative_path_pass_through) {
		char readbuf[32];
		const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);
		EXPECT_EQ(0, tebako_chdir(tmp_path.c_str()));
		int ret = tebako_readlink("link2true", readbuf, num2read);
		EXPECT_EQ(strlen("/bin/true"), ret);
		EXPECT_TRUE(strncmp(readbuf, "/bin/true", ret) == 0);
	}

	TEST_F(LnTests, tebako_lstat_absolute_path) {
		struct stat st;
		int ret = tebako_lstat(TEBAKIZE_PATH("s-link-to-file-1"), &st);
		EXPECT_EQ(0, ret);
		EXPECT_EQ(35, st.st_size);

		ret = tebako_stat(TEBAKIZE_PATH("s-link-to-file-1"), &st);
		EXPECT_EQ(0, ret);
		EXPECT_EQ(37, st.st_size);
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
		ret = tebako_stat("s-link-to-file-1", &st);
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
		int ret = tebako_lstat((tmp_path / "link2true").c_str(), &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(LnTests, tebako_lstat_relative_path_pass_through) {
		struct stat st;
		int ret = tebako_chdir(tmp_path.c_str());
		EXPECT_EQ(0, ret);
		ret = tebako_lstat("link2false", &st);
		EXPECT_EQ(0, ret);
	}

	TEST_F(LnTests, tebako_stat_link_outside_of_memfs) {
		if (!cross_test) {
			struct stat st;
			int ret = tebako_stat(TEBAKIZE_PATH("s-link-outside-of-memfs"), &st);
			EXPECT_EQ(0, ret);
		}
		else {
			GTEST_SKIP();
		}
	}

	TEST_F(LnTests, tebako_open_link_outside_of_memfs) {
		if (!cross_test) {
			int fh = tebako_open(2, TEBAKIZE_PATH("s-link-outside-of-memfs"), O_RDONLY);
			EXPECT_LT(0, fh);

			char readbuf[32];
			const int num2read = sizeof(readbuf) / sizeof(readbuf[0]);

			int ret = tebako_read(fh, readbuf, num2read);
			EXPECT_EQ(num2read, ret);
			EXPECT_EQ(0, strncmp(readbuf, "This is just a file outside of memfs that will be symlinked from inside memfs", num2read));

			ret = tebako_close(fh);
			EXPECT_EQ(0, ret);
		}
		else {
			GTEST_SKIP();
		}
	}

	TEST_F(LnTests, tebako_open_dir_outside_of_memfs) {
		if (!cross_test) {
			DIR* dirp = tebako_opendir(TEBAKIZE_PATH("s-dir-outside-of-memfs"));
			EXPECT_TRUE(dirp != NULL);
			if (dirp != NULL) {
				struct dirent* entry;
				std::string fname = "a-file-outside-of-memfs.txt";
				bool found = false;
				while ((entry= tebako_readdir(dirp)) != NULL) {
					if (fname == entry->d_name) found = true;
				}
				EXPECT_TRUE(found);
			}
		}
		else {
			GTEST_SKIP();
		}
	}

	TEST_F(LnTests, tebako_scan_dir_outside_of_memfs) {
		if (!cross_test) {
			std::string fname = "a-file-outside-of-memfs.txt";
			bool found = false;
			struct dirent** namelist;
			int n = tebako_scandir(TEBAKIZE_PATH("s-dir-outside-of-memfs"), &namelist, NULL, alphasort);
			EXPECT_EQ(n, 3);
			EXPECT_TRUE(namelist != NULL);
			if (n > 0 && namelist != NULL) {
				for (int i = 0; i < n; i++) {
					EXPECT_TRUE(namelist[i] != NULL);
					if (namelist[i]) {
						if (fname == namelist[i]->d_name) found = true;
						free(namelist[i]);
					}
				}
				free(namelist);
			}
			EXPECT_TRUE(found);
		}
		else {
			GTEST_SKIP();
		}
	}

#endif
}
