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

#include <tebako-pch.h>
#include <tebako-common.h>
#include <tebako-pch-pp.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-fd.h>
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

typedef std::set<uintptr_t> tebako_kfdtable;

class sync_tebako_kfdtable : public folly::Synchronized<tebako_kfdtable*> {
public:
	sync_tebako_kfdtable(void) : folly::Synchronized<tebako_kfdtable*>(new tebako_kfdtable) { }

	bool check(uintptr_t dirp) {
		auto p_kfdtable = *rlock();
		auto p_kfd = p_kfdtable->find(dirp);
		return (p_kfd != p_kfdtable->end());
	};

	void erase(uintptr_t dirp) {
		auto p_kfdtable = *wlock();
		p_kfdtable->erase(dirp);
	}

	void insert(uintptr_t dirp) {
		auto p_kfdtable = *wlock();
		p_kfdtable->insert(dirp);
	}

	static sync_tebako_kfdtable kfdtable;
};

sync_tebako_kfdtable sync_tebako_kfdtable::kfdtable;

extern "C" DIR* tebako_opendir(const char* dirname) {
	DIR* ret = NULL;
	if (dirname == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, dirname);

		if (!p_path) {
			ret = ::opendir(dirname);
			if (ret != NULL) {
				sync_tebako_kfdtable::kfdtable.insert(reinterpret_cast<uintptr_t>(ret));
			}
		}
		else {
			int vfd = sync_tebako_fdtable::fdtable.open(p_path, O_RDONLY | O_DIRECTORY);
			if (vfd < 0) {
				if (vfd == DWARFS_INVALID_FD) {
					TEBAKO_SET_LAST_ERROR(ENOENT);
				}
				ret = NULL;
			}
			else {
				ret = reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd));
			}
		}
	}
	return ret;
}

extern "C" DIR* tebako_fdopendir(int vfd) {
	DIR* ret = reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd));
	if (ret == NULL) {
		ret = ::fdopendir(vfd);
		if (ret != NULL) {
			sync_tebako_kfdtable::kfdtable.insert(reinterpret_cast<uintptr_t>(ret));
		}
	}
	return ret;
}

extern "C" int tebako_closedir(DIR * dirp) {
	int ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.closedir(uip);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			ret = DWARFS_IO_ERROR;
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
			ret = ::closedir(dirp);
			sync_tebako_kfdtable::kfdtable.erase(uip);
		}
	}
	return ret;
}

extern "C" struct dirent* tebako_readdir(DIR* dirp) {
	struct dirent* entry = NULL;
	int ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.readdir(uip, entry);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			ret = DWARFS_IO_ERROR;
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
			entry = ::readdir(dirp);
		}
	}
	return entry;
}

extern "C" long tebako_telldir(DIR* dirp) {
	long ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.telldir(uip);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			ret = DWARFS_IO_ERROR;
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
			ret = ::telldir(dirp);
		}
	}
	return ret;
}

extern "C" void tebako_seekdir(DIR* dirp, long loc) {
	int ret = DWARFS_IO_ERROR;
	uintptr_t uip = reinterpret_cast<uintptr_t>(dirp);
	ret = sync_tebako_dstable::dstable.seekdir(uip, loc);
	if (ret == DWARFS_INVALID_FD) {
		if (!sync_tebako_kfdtable::kfdtable.check(uip)) {
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
			::seekdir(dirp, loc);
		}
	}
}

extern "C" void tebako_rewinddir(DIR* dirp) {
	tebako_seekdir(dirp, 0);
}

extern "C" int tebako_dirfd(DIR * dirp) {
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

typedef int(*qsort_compar)(const void*, const void*);

static struct dirent* internal_readdir(DIR* dirp) {
	struct dirent* entry = NULL;
	sync_tebako_dstable::dstable.readdir(reinterpret_cast<uintptr_t>(dirp), entry);
	return entry;
}

extern "C" int tebako_scandir(const char* dirname, struct dirent*** namelist,
    int (*sel)(const struct dirent*),
	int (*compar)(const struct dirent**, const struct dirent**)) {

	int ret = DWARFS_IO_ERROR;
	if (dirname == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		DIR* dirp = NULL;
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, dirname);

		if (!p_path) {
			ret = ::scandir(dirname, namelist, sel, compar);
		}
		else {
			if (namelist != NULL) {
				int vfd = sync_tebako_fdtable::fdtable.open(p_path, O_RDONLY | O_DIRECTORY);
				if (vfd == DWARFS_INVALID_FD) {
					TEBAKO_SET_LAST_ERROR(ENOENT);
					vfd = DWARFS_IO_ERROR;
				}
				if (vfd >= DWARFS_IO_CONTINUE) {
					size_t size;
					dirp = reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd, size));
					if (dirp != NULL) {
						int n = 0;
						struct dirent** list = (dirent**) malloc( sizeof(struct dirent) *size);
						struct dirent* ent = 0, * p = 0;
						while (list != NULL && (ent = internal_readdir(dirp)) != NULL) {
							if (sel && !sel(ent)) continue;
							p = (struct dirent*)malloc(ent->d_reclen);
							if (p == NULL) {
								while (--n >= 0) {
									delete list[n];
								}
								delete[] list;
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
								qsort((void*)*namelist, n, sizeof(struct dirent*), (qsort_compar)compar);
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