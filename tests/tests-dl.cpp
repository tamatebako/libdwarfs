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

#if not (__MACH__ || __musl__)
#include <gnu/lib-names.h>
#endif

namespace {
	class DlTests : public testing::Test {
	protected:
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
		}

		static void TearDownTestSuite() {
			drop_fs();
		}
	};

	TEST_F(DlTests, tebako_dlopen_no_file) {
		errno = 0;
		void* dlptr = tebako_dlopen(TEBAKIZE_PATH("no_file"), RTLD_LAZY | RTLD_GLOBAL);
		EXPECT_EQ(dlptr, nullptr);
		EXPECT_EQ(ENOENT, errno);
	}

	TEST_F(DlTests, tebako_dlopen_no_file_pass_through) {
		errno = 0;
		void* dlptr = tebako_dlopen("/bin/no_file", RTLD_LAZY | RTLD_GLOBAL);
		EXPECT_EQ(dlptr, nullptr);
		EXPECT_NE(tebako_dlerror(), nullptr);
	}

	TEST_F(DlTests, tebako_dlopen_absolute_path) {
		void *handle;
		handle = tebako_dlopen(TEBAKIZE_PATH("directory-1/libempty.so"), RTLD_LAZY | RTLD_GLOBAL);
		EXPECT_NE(handle, nullptr);
        if (handle != nullptr) {
			EXPECT_EQ(0, dlclose(handle));
		}
	}

	TEST_F(DlTests, tebako_dlopen_relative_path) {
		EXPECT_EQ(0,tebako_chdir(TEBAKIZE_PATH("directory-1")));
		void* handle;
		handle = tebako_dlopen("libempty.so", RTLD_LAZY | RTLD_GLOBAL);
		EXPECT_TRUE(handle != NULL);
		if (handle != nullptr) {
			EXPECT_EQ(0, dlclose(handle));
		}
	}

	TEST_F(DlTests, tebako_dlopen_relative_path_dot_dot) {
		EXPECT_EQ(0, tebako_chdir(TEBAKIZE_PATH("directory-3/level-1/level-2///")));
		void* handle;
		handle = tebako_dlopen("../../../directory-1/libempty.so", RTLD_LAZY | RTLD_GLOBAL);
		EXPECT_NE(handle, nullptr);
		if (handle != nullptr) {
			EXPECT_EQ(0, dlclose(handle));
		}
	}

	TEST_F(DlTests, tebako_dlopen_pass_through) {
		void* handle;
		double (*sqrt)(double);
		char* error;
#if __MACH__
        handle = dlopen("/usr/lib/libSystem.dylib", RTLD_LAZY | RTLD_GLOBAL);
#elif __musl__
 		handle = dlopen("/usr/lib/libc.so", RTLD_LAZY | RTLD_GLOBAL);
#else
		handle = dlopen(LIBM_SO, RTLD_LAZY | RTLD_GLOBAL);
#endif
		EXPECT_NE(handle, nullptr);

		// sqrt = (double (*)(double)) dlsym(handle, "sqrt");

		/* According to the ISO C standard, casting between function
		   pointers and 'void *', as done above, produces undefined results.
		   POSIX.1-2001 and POSIX.1-2008 accepted this state of affairs and
		   proposed the following workaround:

			   *(void **) (&cosine) = dlsym(handle, "cos");

		   This (clumsy) cast conforms with the ISO C standard and will
		   avoid any compiler warnings.

		   The 2013 Technical Corrigendum 1 to POSIX.1-2008 improved matters
		   by requiring that conforming implementations support casting
		   'void *' to a function pointer.  Nevertheless, some compilers
		   (e.g., gcc with the '-pedantic' option) may complain about the
		   cast used in this program. */

		* (void**)(&sqrt) = dlsym(handle, "sqrt");

		double rt = (*sqrt)(4.0);
		EXPECT_TRUE(abs(rt - 2.0) < 1E-8);

//		EXPECT_EQ(0, dlclose(handle));        No close because of RTLD_GLOBAL (?)
	}
}
