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
#include <tebako-io-inner.h>
#include <tebako-dfs.h>

using namespace std;

const size_t TEBAKO_DIR_CACHE_SIZE = 50;

struct tebako_ds {
	tebako_dirent cache[TEBAKO_DIR_CACHE_SIZE];
	size_t dir_size;
	long dir_position;
	off_t cache_start;
	size_t cache_size;
	int vfd;

	tebako_ds(int fd) : cache_size(0), cache_start(0), dir_position(-1), dir_size(0), vfd(fd) {
	}

	int load_cache(int new_cache_start, bool set_pos = false) {
		int ret = dwarfs_fd_readdir(vfd, cache, new_cache_start, TEBAKO_DIR_CACHE_SIZE, cache_size, dir_size);
		if (ret == DWARFS_IO_CONTINUE) {
			if (set_pos) {
				dir_position = new_cache_start;
			}
			cache_start = new_cache_start;
		}
		else {
			dir_position = -1;
			cache_size = 0;
		}
		return ret;
	}

};

typedef map<uintptr_t, shared_ptr<tebako_ds>> tebako_dstable;

class sync_tebako_dstable : public folly::Synchronized<tebako_dstable*> {
public:
	sync_tebako_dstable(void) : folly::Synchronized<tebako_dstable*>(new tebako_dstable) { }

	void close_all(void) {
		auto p_dstable = *this->wlock();
		p_dstable->clear();
	}

	uintptr_t opendir(int vfd) {
		uintptr_t ret = 0;;
		int err = ENOTDIR;
		try {
			auto ds = make_shared<tebako_ds>(tebako_ds(vfd));
			if (ds == NULL) {
				err = ENOMEM;
			}
			else {
				err = ds->load_cache(0, true);
				if (err == DWARFS_IO_CONTINUE) {
					ret = reinterpret_cast<uintptr_t>(ds.get());
					(**dstable.wlock())[ret] = ds;
				}
				else {
					ret = 0;
				}
			}
		}
		catch (...) {
			ret = 0;
			err = EBADF;
		}
		if (ret == 0) {
			TEBAKO_SET_LAST_ERROR(err);
		}
		return ret;
	}

	int closedir(uintptr_t dirp) {
		return ((**dstable.wlock()).erase(dirp) > 0) ? DWARFS_IO_CONTINUE : DWARFS_INVALID_FD;
	}

	long telldir(uintptr_t dirp) {
		long ret = DWARFS_INVALID_FD;
		auto p_dstable = *this->rlock();
		auto p_ds = p_dstable->find(dirp);
		if (p_ds != p_dstable->end()) {
			ret = p_ds->second->dir_position;
		}
		return ret;
	}

	int seekdir(uintptr_t dirp, long pos) {
		long ret = DWARFS_INVALID_FD;
		auto p_dstable = *this->rlock();
		auto p_ds = p_dstable->find(dirp);
		if (p_ds != p_dstable->end()) {
			p_ds->second->dir_position = pos;
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}

	long dirfd(uintptr_t dirp) {
		long ret = DWARFS_INVALID_FD;
		auto p_dstable = *this->rlock();
		auto p_ds = p_dstable->find(dirp);
		if (p_ds != p_dstable->end()) {
			ret = p_ds->second->vfd;
		}
		return ret;
	}

	int readdir(uintptr_t  dirp, struct dirent*& entry) {
		int ret = DWARFS_INVALID_FD; 
		entry = NULL;
		auto p_dstable = *this->rlock();
		auto p_ds = p_dstable->find(dirp);
		if (p_ds != p_dstable->end()) {
			long pos = p_ds->second->dir_position;
			if (p_ds->second->dir_position < 0) {      // Should not happen ever
				ret = DWARFS_IO_ERROR;
				TEBAKO_SET_LAST_ERROR(EBADF);
			}
			else {
				if (p_ds->second->dir_position > p_ds->second->dir_size) {
					ret = DWARFS_IO_CONTINUE;
				}
				else {
					if (p_ds->second->dir_position > p_ds->second->cache_start + p_ds->second->cache_size - 1 ||
						p_ds->second->dir_position < p_ds->second->cache_start) {
						try {
							ret = p_ds->second->load_cache(0, false);
							if (ret == DWARFS_IO_CONTINUE) {
								entry = &p_ds->second->cache[p_ds->second->dir_position - p_ds->second->cache_start].e;
							}
							else {
								TEBAKO_SET_LAST_ERROR(ret);
							}
						}
						catch (...) {
							ret = DWARFS_IO_ERROR;
							TEBAKO_SET_LAST_ERROR(EBADF);
						}
					}
					else {
						entry = &p_ds->second->cache[p_ds->second->dir_position - p_ds->second->cache_start].e;
						ret = DWARFS_IO_CONTINUE;
					}
				}
			}
		}
		return ret;
	}

	static sync_tebako_dstable dstable;
};

sync_tebako_dstable sync_tebako_dstable::dstable;

void dwarfs_dir_close_all(void)  noexcept {
	sync_tebako_dstable::dstable.close_all();
}

DIR* dwarfs_fdopendir(int vfd) noexcept {
	return reinterpret_cast<DIR*>(sync_tebako_dstable::dstable.opendir(vfd));
}

int dwarfs_closedir(DIR* dirp) noexcept {
	return sync_tebako_dstable::dstable.opendir(reinterpret_cast<uintptr_t>(dirp));
}

long dwarfs_telldir(DIR* dirp) noexcept {
	return sync_tebako_dstable::dstable.telldir(reinterpret_cast<uintptr_t>(dirp));
}

int dwarfs_seekdir(DIR* dirp, long pos) noexcept {
	return sync_tebako_dstable::dstable.seekdir(reinterpret_cast<uintptr_t>(dirp), pos);
}

int dwarfs_dirfd(DIR* dirp) noexcept {
	return sync_tebako_dstable::dstable.dirfd(reinterpret_cast<uintptr_t>(dirp));
}

int dwarfs_readdir(DIR* dirp, struct dirent*& entry) {
	return sync_tebako_dstable::dstable.readdir(reinterpret_cast<uintptr_t>(dirp), entry);
}
