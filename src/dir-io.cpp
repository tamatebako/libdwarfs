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

#include <tebako-common.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>

#include <filesystem>

/*
* tebako_opendir
* tebako_fdopendir
* tebako_closedir
* tebako_readdir
* tebako_telldir
* tebako_seekdir
* tebako_rewinddir
* tebako_dirfd
* tebako_scandir
*
*  https://pubs.opengroup.org/onlinepubs/9699919799/
*/

extern "C" DIR* tebako_opendir(const char* dirname) {
	DIR* ret = NULL;
	tebako_path_t t_path;
	const char* p_path = to_tebako_path(t_path, dirname);

	return p_path ? dwarfs_opendir(p_path) : ::opendir(dirname);
}

extern "C" DIR* tebako_fdopendir(int fd) {
	return NULL;
}

extern "C" struct dirent* tebako_readdir(DIR* dirp) {
	return NULL;
}

extern "C" long tebako_telldir(DIR* dirp) {
	return -1;
}

extern "C" void tebako_seekdir(DIR* dirp, long loc) {
}

extern "C" void tebako_rewinddir(DIR* dirp) {
}

extern "C" int tebako_closedir(DIR * dirp) {
	return -1;
}

extern "C" int tebako_dirfd(DIR * dirp) {
	return -1;
}

extern "C" int tebako_scandir(const char* dir, struct dirent*** namelist,
	int (*sel)(const struct dirent*),
	int (*compar)(const struct dirent**, const struct dirent**)) {
	return -1;
}
