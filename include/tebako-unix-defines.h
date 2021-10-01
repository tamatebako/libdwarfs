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

#ifndef TEBAKO_UNIX_DEFINES_H_INCLUDED
#define _TEBAKO_UNIX_DEFINES_H_INCLUDED
#ifndef _WIN32
	#ifdef dirfd
		#undef dirfd
	#endif

	#define getcwd(...)	_tebako_getcwd(__VA_ARGS__)
	#define getwd(...)	_tebako_getwd(__VA_ARGS__)
	#define chdir(...)	_tebako_chdir(__VA_ARGS__)
	#define stat(...)	_tebako_stat(__VA_ARGS__)
	#define fstat(...)	_tebako_fstat(__VA_ARGS__)
	#define lstat(...)	_tebako_lstat(__VA_ARGS__)
	#define open(...)	_tebako_open(_TEBAKO_PP_NARG(__VA_ARGS__), __VA_ARGS__)
	#define openat(...)	_tebako_openat(_TEBAKO_PP_NARG(__VA_ARGS__), __VA_ARGS__)
	#define close(...)	_tebako_close(__VA_ARGS__)
	#define read(...)	_tebako_read(__VA_ARGS__)
	#define lseek(...)	_tebako_lseek(__VA_ARGS__)
	#define readlink(...)	_tebako_readlink(__VA_ARGS__)
	#define opendir(...)	_tebako_opendir(__VA_ARGS__)
	#define fdopendir(...)	_tebako_fdopendir(__VA_ARGS__)
	#define closedir(...)	_tebako_closedir(__VA_ARGS__)
	#define readdir(...)	_tebako_readdir(__VA_ARGS__)
	#define telldir(...)	_tebako_telldir(__VA_ARGS__)
	#define seekdir(...)	_tebako_seekdir(__VA_ARGS__)
	#define rewinddir(...)	_tebako_rewinddir(__VA_ARGS__)
	#define dirfd(...)	_tebako_dirfd(__VA_ARGS__)
	#define scandir(...)	_tebako_scandir(__VA_ARGS__)
	#define pread(...)	_tebako_pread(__VA_ARGS__)
	#define readv(...)	_tebako_readv(__VA_ARGS__)
	#define dlopen(...)	_tebako_dlopen(__VA_ARGS__)
	#define access(...)	_tebako_access(__VA_ARGS__)
	#define mkdir(...)	_tebako_mkdir(__VA_ARGS__)     

#endif // !_WIN32
#endif // _TEBAKO_UNIX_DEFINES_H_INCLUDED
