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

 /*
 *  C functions redifinitions
 *  This file shall be included into sources we want to "hack"
 */


#pragma once

#define _TEBAKO_PP_NARG(...) \
    _TEBAKO_PP_NARG_(__VA_ARGS__,_TEBAKO_PP_RSEQ_N())
#define _TEBAKO_PP_NARG_(...) \
    _TEBAKO_PP_ARG_N(__VA_ARGS__)
#define _TEBAKO_PP_ARG_N( \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
    _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
    _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
    _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
    _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
    _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
    _61,_62,_63,  N, ...) N
#define _TEBAKO_PP_RSEQ_N() \
    63,62,61,60,                   \
    59,58,57,56,55,54,53,52,51,50, \
    49,48,47,46,45,44,43,42,41,40, \
    39,38,37,36,35,34,33,32,31,30, \
    29,28,27,26,25,24,23,22,21,20, \
    19,18,17,16,15,14,13,12,11,10, \
     9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#ifndef _WIN32
 #ifdef dirfd
  #undef dirfd
 #endif

 #define getcwd(...)	tebako_getcwd(__VA_ARGS__)

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
 #define getwd(...)	tebako_getwd(__VA_ARGS__)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

 #define chdir(...)         tebako_chdir(__VA_ARGS__)
 #define stat(...)          tebako_stat(__VA_ARGS__)
 #define fstat(...)         tebako_fstat(__VA_ARGS__)
 #define lstat(...)         tebako_lstat(__VA_ARGS__)
 #define fstatat(...)       tebako_fstatat(__VA_ARGS__)
 #define open(...)          tebako_open(_TEBAKO_PP_NARG(__VA_ARGS__), __VA_ARGS__)
 #define openat(...)        tebako_openat(_TEBAKO_PP_NARG(__VA_ARGS__), __VA_ARGS__)
 #define close(...)         tebako_close(__VA_ARGS__)
 #define read(...)          tebako_read(__VA_ARGS__)
 #define lseek(...)         tebako_lseek(__VA_ARGS__)
 #define readlink(...)      tebako_readlink(__VA_ARGS__)
 #define opendir(...)       tebako_opendir(__VA_ARGS__)
 #define fdopendir(...)     tebako_fdopendir(__VA_ARGS__)
 #define closedir(...)      tebako_closedir(__VA_ARGS__)
 #define readdir(...)       tebako_readdir(__VA_ARGS__)
 #define telldir(...)       tebako_telldir(__VA_ARGS__)
 #define seekdir(...)       tebako_seekdir(__VA_ARGS__)
 #define rewinddir(...)     tebako_rewinddir(__VA_ARGS__)
 #define dirfd(...)         tebako_dirfd(__VA_ARGS__)
 #define scandir(...)       tebako_scandir(__VA_ARGS__)
 #define pread(...)         tebako_pread(__VA_ARGS__)
 #define readv(...)         tebako_readv(__VA_ARGS__)
 #define dlopen(...)        tebako_dlopen(__VA_ARGS__)
 #define access(...)        tebako_access(__VA_ARGS__)
 #define mkdir(...)         tebako_mkdir(__VA_ARGS__)

#endif // !_WIN32

