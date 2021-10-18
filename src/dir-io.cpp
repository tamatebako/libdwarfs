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
#include <tebako-pch-pp.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-dirent.h>

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

	if (!p_path) {
		ret = ::opendir(dirname);
	}
	else {
		int vfd = dwarfs_open(p_path, O_RDONLY|O_DIRECTORY);
		if (vfd == DWARFS_INVALID_FD) {
			TEBAKO_SET_LAST_ERROR(ENOENT);
		}
		ret = (vfd < 0) ? NULL : reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd));
	}
	return ret;
}

extern "C" DIR* tebako_fdopendir(int vfd) {
	DIR* ret = reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd));
	return (ret == NULL) ? ::fdopendir(vfd) : ret;
}

extern "C" int tebako_closedir(DIR * dirp) {
	int ret = sync_tebako_dstable::dstable.closedir(reinterpret_cast<uintptr_t>(dirp));
	return (ret == DWARFS_INVALID_FD) ? ::closedir(dirp) : ret;
}

extern "C" struct dirent* tebako_readdir(DIR* dirp) {
	struct dirent* entry = NULL;
	int ret = sync_tebako_dstable::dstable.readdir(reinterpret_cast<uintptr_t>(dirp), entry);
	return (ret == DWARFS_INVALID_FD) ? ::readdir(dirp) : entry;
}

extern "C" long tebako_telldir(DIR* dirp) {
	long ret = sync_tebako_dstable::dstable.telldir(reinterpret_cast<uintptr_t>(dirp));
	return (ret == DWARFS_INVALID_FD) ? ::telldir(dirp) : ret;
}

extern "C" void tebako_seekdir(DIR* dirp, long loc) {
	int ret = sync_tebako_dstable::dstable.seekdir(reinterpret_cast<uintptr_t>(dirp), loc);
	if (ret == DWARFS_INVALID_FD) {
		::seekdir(dirp, loc);
	}
}

extern "C" void tebako_rewinddir(DIR* dirp) {
	tebako_seekdir(dirp, 0);
}

extern "C" int tebako_dirfd(DIR * dirp) {
	int ret = sync_tebako_dstable::dstable.dirfd(reinterpret_cast<uintptr_t>(dirp));
	return (ret == DWARFS_INVALID_FD) ? ::dirfd(dirp) : ret;
}

typedef int(*qsort_compar)(const void*, const void*);

 static struct dirent* internal_readdir(DIR* dirp) {
	 struct dirent* entry = NULL;
	 sync_tebako_dstable::dstable.readdir(reinterpret_cast<uintptr_t>(dirp), entry);
	 return entry;
 }

 extern "C" int tebako_scandir(const char* dir, struct dirent*** namelist,
	 int (*sel)(const struct dirent*),
	 int (*compar)(const struct dirent**, const struct dirent**)) {

	 int ret = -1;
	 DIR* dirp = NULL;
	 tebako_path_t t_path;
	 const char* p_path = to_tebako_path(t_path, dir);

	 if (!p_path) {
		 ret = ::scandir(dir, namelist, sel, compar);
	 }
	 else {
		 if (namelist != NULL) {
			 int vfd = dwarfs_open(p_path, O_RDONLY | O_DIRECTORY);
			 if (vfd == DWARFS_INVALID_FD) {
				 TEBAKO_SET_LAST_ERROR(ENOENT);
				 vfd = DWARFS_IO_ERROR;
			 }
			 if (vfd >= DWARFS_IO_CONTINUE) {
				 size_t size;
				 dirp = reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd, size));
				 if (dirp != NULL) {
					 int n = 0;
					 struct dirent** list = new struct dirent *[size]; 
					 struct dirent* ent = 0, * p = 0;
					 while (list != NULL && (ent = internal_readdir(dirp)) != NULL) {
						 if (sel && !sel(ent)) continue;
						 p = (struct dirent*)malloc(ent->d_reclen);
						 if (p == NULL) {
							 while (--n >= 0) {
								 delete list[n];
							 }
							delete list;
							list = NULL;
						 }
						 else {
							 memcpy((void*)p, (void*)ent, ent->d_reclen);
							 list[n++] = p;
						 }
					 }
					 sync_tebako_dstable::dstable.closedir(reinterpret_cast<uintptr_t>(dirp));
					 if (list == NULL) {
						 TEBAKO_SET_LAST_ERROR(ENOMEM);
					 }
					 else {
						 *namelist = (struct dirent**)realloc((void*)list, std::max(n,1) * sizeof(struct dirent*));
						 if (*namelist == NULL) {
							 *namelist = list;
						 }
						 if (compar && n > 0) {
							 qsort((void*)*namelist, n, sizeof(struct dirent), (qsort_compar)compar);
						 }
						 ret = n;
					 }
				 }
			 }
		 }
	 }
	 return ret;
 }
