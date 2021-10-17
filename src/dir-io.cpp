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
		ret = (vfd < 0) ? NULL : dwarfs_fdopendir(vfd); 
	}
	return ret;
}

extern "C" DIR* tebako_fdopendir(int vfd) {
	DIR* ret = dwarfs_fdopendir(vfd);
	return (ret == NULL) ? ::fdopendir(vfd) : ret;
}

extern "C" int tebako_closedir(DIR * dirp) {
	int ret = dwarfs_closedir(dirp);
	return (ret == DWARFS_INVALID_FD) ? ::closedir(dirp) : ret;
}


extern "C" struct dirent* tebako_readdir(DIR* dirp) {
	struct dirent* res = NULL;
	int ret = dwarfs_readdir(dirp, res);
	return (ret == DWARFS_INVALID_FD) ? ::readdir(dirp) : res;
}

extern "C" long tebako_telldir(DIR* dirp) {
	long ret = dwarfs_telldir(dirp);
	return (ret == DWARFS_INVALID_FD) ? ::telldir(dirp) : ret;
}

extern "C" void tebako_seekdir(DIR* dirp, long loc) {
	int ret = dwarfs_seekdir(dirp, loc);
	if (ret == DWARFS_INVALID_FD) {
		::seekdir(dirp, loc);
	}
}

extern "C" void tebako_rewinddir(DIR* dirp) {
	tebako_seekdir(dirp, 0);
}

extern "C" int tebako_dirfd(DIR * dirp) {
	int ret = dwarfs_dirfd(dirp);
	return (ret == DWARFS_INVALID_FD) ? ::dirfd(dirp) : ret;
}

extern "C" int tebako_scandir(const char* dir, struct dirent*** namelist,
	int (*sel)(const struct dirent*),
	int (*compar)(const struct dirent**, const struct dirent**)) {
	return -1;
}

/*
+ typedef int(*qsort_compar)(const void*, const void*);
+
+int squash_scandir(sqfs * fs, const char* dirname, struct SQUASH_DIRENT*** namelist,
	+int (*select)(const struct SQUASH_DIRENT*),
	+int (*compar)(const struct SQUASH_DIRENT**, const struct SQUASH_DIRENT**))
	+ {
	+SQUASH_DIR * openeddir = 0;
	+size_t n = 0;
	+struct SQUASH_DIRENT** list = NULL;
	+struct SQUASH_DIRENT* ent = 0, *p = 0;
	+
		+if ((dirname == NULL) || (namelist == NULL))
		+ return -1;
	+
		+openeddir = squash_opendir(fs, dirname);
	+if (openeddir == NULL)
		+ return -1;
	+
		+
		+list = (struct SQUASH_DIRENT**)malloc(MAX_DIR_ENT * sizeof(struct SQUASH_DIRENT*));
	+
		+
		+while ((ent = squash_readdir(openeddir)) != NULL)
		+ {
		+if (select && !select(ent))
			+ continue;
		+
			+p = (struct SQUASH_DIRENT*)malloc(sizeof(struct SQUASH_DIRENT));
		+
			+memcpy((void*)p, (void*)ent, sizeof(struct SQUASH_DIRENT));
		+list[n] = p;
		+
			+n++;
		+if (n >= MAX_DIR_ENT)
			+ break;
		+
			+}
	+
		+	//close the squash_dir
		+squash_closedir(openeddir);
	+
		+	//realloc the array
		+*namelist = realloc((void*)list, n * sizeof(struct SQUASH_DIRENT*));
	+if (*namelist == NULL)
		+ *namelist = list;
	+
		+
		+	//sort the array
		+if (compar)
		+qsort((void*)*namelist, n, sizeof(struct SQUASH_DIRENT*), (qsort_compar)compar);
	+
		+return n;
	+
		+}

*/