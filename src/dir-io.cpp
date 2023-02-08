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

#include <tebako-pch.h>
#ifndef RB_W32
#include <dirent.h>
#endif
#include <tebako-pch-pp.h>
#include <tebako-common.h>
#include <tebako-dirent.h>
#include <tebako-io-rb-w32.h>
#include <tebako-io-rb-w32-inner.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-fd.h>
#include <tebako-kdf.h>

sync_tebako_kfdtable sync_tebako_kfdtable::kfdtable;

DIR* tebako_opendir(const char* dirname) {
	DIR* ret = NULL;
	if (dirname == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		int vfd;
		tebako_path_t t_path;
		std::string r_dirname = dirname;
		const char* p_path = to_tebako_path(t_path, dirname);

		if (p_path) {
			vfd = sync_tebako_fdtable::fdtable.open(p_path, O_RDONLY | O_DIRECTORY, r_dirname);
			switch(vfd) {
				case DWARFS_S_LINK_OUTSIDE:
					break;
				case DWARFS_INVALID_FD:
					ret = NULL;
					TEBAKO_SET_LAST_ERROR(ENOENT);
					break;
				case DWARFS_IO_ERROR:
					ret = NULL;
					break;
				default:
					ret = reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd));
					break;
			}
		}

		if (!p_path || vfd == DWARFS_S_LINK_OUTSIDE) {
			ret = TO_RB_W32_U(opendir)(r_dirname.c_str());
			if (ret != NULL) {
				sync_tebako_kfdtable::kfdtable.insert(reinterpret_cast<uintptr_t>(ret));
			}
		}
	}
	return ret;
}

#ifdef TEBAKO_HAS_FDOPENDIR
DIR* tebako_fdopendir(int vfd) {
	DIR* ret = reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd));
	if (ret == NULL) {
		ret = ::fdopendir(vfd);
		if (ret != NULL) {
			sync_tebako_kfdtable::kfdtable.insert(reinterpret_cast<uintptr_t>(ret));
		}
	}
	return ret;
}
#endif

int tebako_closedir(DIR * dirp) {
	int ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.closedir(uip);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			ret = DWARFS_IO_ERROR;
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
#ifdef RB_W32
			ret = DWARFS_IO_CONTINUE;
#else
			ret =
#endif
			TO_RB_W32(closedir)(dirp);
			sync_tebako_kfdtable::kfdtable.erase(uip);
		}
	}
	return ret;
}

#ifdef RB_W32
struct direct* tebako_readdir(DIR* dirp, void * enc) {
	struct direct* entry = NULL;
#else
struct dirent* tebako_readdir(DIR* dirp) {
	struct dirent* entry = NULL;
#endif
	tebako_dirent* e = NULL;
	int ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.readdir(uip, e);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			ret = DWARFS_IO_ERROR;
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
#ifdef RB_W32
			entry = TO_RB_W32(readdir)(dirp, enc);
#else
			entry = TO_RB_W32(readdir)(dirp);
#endif
		}
	}
	else {
		entry = &e->e;
	}
	return entry;
}

long tebako_telldir(DIR* dirp) {
	long ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.telldir(uip);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			ret = DWARFS_IO_ERROR;
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
			ret = TO_RB_W32(telldir)(dirp);
		}
	}
	return ret;
}

void tebako_seekdir(DIR* dirp, long loc) {
	int ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.seekdir(uip, loc);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
			TO_RB_W32(seekdir)(dirp, loc);
		}
	}
}

#ifdef TEBAKO_HAS_DIRFD
int tebako_dirfd(DIR * dirp) {
	int ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.dirfd(uip);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			ret = DWARFS_IO_ERROR;
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
			ret = ::dirfd(dirp);
		}
	}
	return ret;
}
#endif

#ifdef TEBAKO_HAS_SCANDIR
typedef int(*qsort_compar)(const void*, const void*);

static struct dirent* internal_readdir(DIR* dirp) {
	tebako_dirent* entry = NULL;
	sync_tebako_dstable::dstable.readdir(reinterpret_cast<uintptr_t>(dirp), entry);
	return &entry->e;
}

int tebako_scandir(const char* dirname, struct dirent*** namelist,
    int (*sel)(const struct dirent*),
	int (*compar)(const struct dirent**, const struct dirent**)) {

	int ret = DWARFS_IO_ERROR;
	if (dirname == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		int vfd;
		std::string dirname_r = dirname;
		DIR* dirp = NULL;
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, dirname);

		if (p_path) {
			vfd = sync_tebako_fdtable::fdtable.open(p_path, O_RDONLY | O_DIRECTORY, dirname_r);
		}
		if (!p_path || vfd == DWARFS_S_LINK_OUTSIDE) {
			ret = ::scandir(dirname_r.c_str(), namelist, sel, compar);
		}
		else {
			if (namelist != NULL) {
				if (vfd == DWARFS_INVALID_FD) {
					TEBAKO_SET_LAST_ERROR(ENOENT);
					vfd = DWARFS_IO_ERROR;
				}
				if (vfd >= DWARFS_IO_CONTINUE) {
					size_t size;
					dirp = reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd, size));
					if (dirp != NULL) {
						int n = 0;
						dirent** list = (dirent**) malloc( sizeof(dirent*) *size);
						dirent* ent = 0, * p = 0;
						while (list != NULL && (ent = internal_readdir(dirp)) != NULL) {
							if (sel && !sel(ent)) continue;
							p = (struct dirent*)malloc(ent->d_reclen);
							if (p == NULL) {
								while (--n >= 0) {
									free(list[n]);
								}
								free(list);
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
							*namelist = (struct dirent**)realloc((void*)list, std::max(n, 1) * sizeof(struct dirent*));
							if (*namelist == NULL) {
								*namelist = list;
							}
							if (compar && n > 0) {
								qsort((void*)*namelist, n, sizeof(dirent*), (qsort_compar)compar);
							}
							ret = n;
						}
					}
				}
			}
			else {
				// This is not POSIX but posix does not cover this case (namelist==NULL) at all
				TEBAKO_SET_LAST_ERROR(EFAULT);
			}
		}
	}
	return ret;
}
#endif
