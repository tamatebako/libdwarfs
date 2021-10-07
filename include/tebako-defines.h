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

#pragma once


#ifndef _WIN32
 #ifdef dirfd
  #undef dirfd
 #endif

 #define getcwd(...)	tebako_getcwd(__VA_ARGS__)
 #define getwd(...)	tebako_getwd(__VA_ARGS__)
 #define chdir(...)	tebako_chdir(__VA_ARGS__)
 #define stat(a1,a2)	tebako_stat(a1,a2)
 #define fstat(...)	tebako_fstat(__VA_ARGS__)
 #define lstat(...)	tebako_lstat(__VA_ARGS__)
 #define open(...)	tebako_open(_TEBAKO_PP_NARG(__VA_ARGS__), __VA_ARGS__)
 #define openat(...)	tebako_openat(_TEBAKO_PP_NARG(__VA_ARGS__), __VA_ARGS__)
 #define close(...)	tebako_close(__VA_ARGS__)
 #define read(...)	tebako_read(__VA_ARGS__)
 #define lseek(...)	tebako_lseek(__VA_ARGS__)
 #define readlink(...)	tebako_readlink(__VA_ARGS__)
 #define opendir(...)	tebako_opendir(__VA_ARGS__)
 #define fdopendir(...)	tebako_fdopendir(__VA_ARGS__)
 #define closedir(...)	tebako_closedir(__VA_ARGS__)
 #define readdir(...)	tebako_readdir(__VA_ARGS__)
 #define telldir(...)	tebako_telldir(__VA_ARGS__)
 #define seekdir(...)	tebako_seekdir(__VA_ARGS__)
 #define rewinddir(...)	tebako_rewinddir(__VA_ARGS__)
 #define dirfd(...)	tebako_dirfd(__VA_ARGS__)
 #define scandir(...)	tebako_scandir(__VA_ARGS__)
 #define pread(...)	tebako_pread(__VA_ARGS__)
 #define readv(...)	tebako_readv(__VA_ARGS__)
 #define dlopen(...)	tebako_dlopen(__VA_ARGS__)
 #define access(...)	tebako_access(__VA_ARGS__)
 #define mkdir(...)	tebako_mkdir(__VA_ARGS__)     

#endif // !_WIN32

