/**
 *
 * Copyright (c) 2021-2025, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of the Tebako project. (libdwarfs-wr)
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
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "tests.h"

#if not(__MACH__ || __musl__ || _WIN32)
#include <gnu/lib-names.h>
#endif

namespace {
class DlTests : public testing::Test {
 protected:
#ifdef _WIN32
  static void invalidParameterHandler(const wchar_t* p1,
                                      const wchar_t* p2,
                                      const wchar_t* p3,
                                      unsigned int p4,
                                      uintptr_t p5)
  {
    // Just return to pass execution to standard library
    // otherwise exception will be thrown by MSVC runtime
    return;
  }
#endif

  static void SetUpTestSuite()
  {
#ifdef _WIN32
    _set_invalid_parameter_handler(invalidParameterHandler);
#endif
    mount_root_memfs(&gfsData[0], gfsSize, tests_log_level(), NULL /* cachesize*/, NULL /* workers */, NULL /* mlock */,
                     NULL /* decompress_ratio*/, NULL /* image_offset */
    );
  }

  static void TearDownTestSuite()
  {
    unmount_root_memfs();
  }
};

TEST_F(DlTests, tebako_dlopen_null)
{
  errno = 0;
  void* handle = tebako_dlopen(nullptr, RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_NE(handle, nullptr);
  if (handle != nullptr) {
    EXPECT_EQ(0, dlclose(handle));
  }
}

TEST_F(DlTests, tebako_dlopen_no_file)
{
  errno = 0;
  void* dlptr = tebako_dlopen(TEBAKIZE_PATH("no_file"), RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_EQ(dlptr, nullptr);
  char* r = tebako_dlerror();
  std::string expected =
      std::string(TEBAKIZE_PATH("no_file")) + ": cannot open shared object file: No such file or directory";
  std::replace(expected.begin(), expected.end(), '\\', '/');
  EXPECT_EQ(expected, r);
}

TEST_F(DlTests, tebako_dlopen_no_file_pass_through)
{
  errno = 0;
  void* dlptr = tebako_dlopen(__AT_BIN__("no_file"), RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_EQ(dlptr, nullptr);
  EXPECT_NE(tebako_dlerror(),
            nullptr);  // Specific otput cannot be guaranteed accross diffrent OSs
}

TEST_F(DlTests, tebako_dlopen_absolute_path)
{
  void* handle;
  handle = tebako_dlopen(TEBAKIZE_PATH("directory-1/" __LIBEMPTY__), RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_NE(handle, nullptr);
  if (handle != nullptr) {
    EXPECT_EQ(0, dlclose(handle));
  }
}

TEST_F(DlTests, tebako_dlopen_twice)
{
  void *handle1, *handle2;
  handle1 = tebako_dlopen(TEBAKIZE_PATH("directory-1/" __LIBEMPTY__), RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_NE(handle1, nullptr);

  handle2 = tebako_dlopen(TEBAKIZE_PATH("directory-1/" __LIBEMPTY__), RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_NE(handle2, nullptr);

  EXPECT_EQ(handle1, handle2);

  if (handle1 != nullptr) {
    EXPECT_EQ(0, dlclose(handle1));
  }

  if (handle2 != nullptr) {
    EXPECT_EQ(0, dlclose(handle2));
  }
}

#ifdef _WIN32
TEST_F(DlTests, tebako_dlopen_absolute_path_win)
{
  void* handle;
  handle = tebako_dlopen(TEBAKIZE_PATH("directory-1\\" __LIBEMPTY__), RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_NE(handle, nullptr);
  if (handle != nullptr) {
    EXPECT_EQ(0, dlclose(handle));
  }
}
#endif

TEST_F(DlTests, tebako_dlopen_relative_path)
{
  EXPECT_EQ(0, tebako_chdir(TEBAKIZE_PATH("directory-1")));
  void* handle;
  handle = tebako_dlopen(__LIBEMPTY__, RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_TRUE(handle != NULL);
  if (handle != nullptr) {
    EXPECT_EQ(0, dlclose(handle));
  }
}

TEST_F(DlTests, tebako_dlopen_relative_path_dot_dot)
{
  EXPECT_EQ(0, tebako_chdir(TEBAKIZE_PATH("directory-3/level-1/level-2///")));
  void* handle;
  handle = tebako_dlopen("../../../directory-1/" __LIBEMPTY__, RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_NE(handle, nullptr);
  if (handle != nullptr) {
    EXPECT_EQ(0, dlclose(handle));
  }
}

#ifdef _WIN32
TEST_F(DlTests, tebako_dlopen_relative_path_dot_dot_win)
{
  EXPECT_EQ(0, tebako_chdir(TEBAKIZE_PATH("directory-3\\level-1\\level-2\\\\\\")));
  void* handle;
  handle = tebako_dlopen("..\\..\\..\\directory-1\\" __LIBEMPTY__, RTLD_LAZY | RTLD_GLOBAL);
  EXPECT_NE(handle, nullptr);
  if (handle != nullptr) {
    EXPECT_EQ(0, dlclose(handle));
  }
}
#endif

TEST_F(DlTests, tebako_dlopen_pass_through)
{
  void* handle;
  double (*sqrt)(double);
  char* error;
#if __MACH__
  handle = tebako_dlopen("/usr/lib/libSystem.dylib", RTLD_LAZY | RTLD_GLOBAL);
#elif __musl__
  handle = tebako_dlopen("/usr/lib/libc.so", RTLD_LAZY | RTLD_GLOBAL);
#elif defined(_WIN32)
  handle = tebako_dlopen(__AT_BIN__("msvcrt.dll"), RTLD_LAZY | RTLD_GLOBAL);
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

  *(void**)(&sqrt) = dlsym(handle, "sqrt");

  double rt = (*sqrt)(4.0);
  EXPECT_TRUE(abs(rt - 2.0) < 1E-8);

  //		EXPECT_EQ(0, dlclose(handle));        No close because of
  // RTLD_GLOBAL (?)
}

TEST_F(DlTests, tebako_dlmap2file_no_file)
{
  errno = 0;
  char* dlmapped = tebako_dlmap2file(TEBAKIZE_PATH("no_file"));
  EXPECT_EQ(dlmapped, nullptr);
  char* r = tebako_dlerror();
  std::string expected =
      std::string(TEBAKIZE_PATH("no_file")) + ": cannot open shared object file: No such file or directory";
  std::replace(expected.begin(), expected.end(), '\\', '/');
  EXPECT_EQ(expected, r);
}

TEST_F(DlTests, tebako_dlmap2file_absolute_path)
{
  char* dlmapped = tebako_dlmap2file(TEBAKIZE_PATH("directory-1/" __LIBEMPTY__));
  EXPECT_NE(dlmapped, nullptr);
  if (dlmapped != nullptr) {
    void* handle = ::dlopen(dlmapped, RTLD_LAZY | RTLD_GLOBAL);
    EXPECT_NE(handle, nullptr);
    if (handle != nullptr) {
      ::dlclose(handle);
    }
    free(dlmapped);
  }
}

TEST_F(DlTests, tebako_dlmap2file_relative_path)
{
  EXPECT_EQ(0, tebako_chdir(TEBAKIZE_PATH("directory-1")));
  char* dlmapped = tebako_dlmap2file(__LIBEMPTY__);
  EXPECT_NE(dlmapped, nullptr);
  if (dlmapped != nullptr) {
    void* handle = ::dlopen(dlmapped, RTLD_LAZY | RTLD_GLOBAL);
    EXPECT_NE(handle, nullptr);
    if (handle != nullptr) {
      ::dlclose(handle);
    }
    free(dlmapped);
  }
}

TEST_F(DlTests, tebako_dlmap2file_pass_through)
{
  char* dlmapped;
#if __MACH__
  dlmapped = tebako_dlmap2file("/usr/lib/libSystem.dylib");
#elif __musl__
  dlmapped = tebako_dlmap2file("/usr/lib/libc.so");
#elif defined(_WIN32)
  dlmapped = tebako_dlmap2file(__AT_BIN__("msvcrt.dll"));
#else
  dlmapped = tebako_dlmap2file(LIBM_SO);
#endif
  EXPECT_EQ(dlmapped, nullptr);
}

}  // namespace
