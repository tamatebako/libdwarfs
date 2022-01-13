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
#include <tebako-common.h>
#include <tebako-pch-pp.h>
#include <tebako-io-inner.h>
#include <tebako-fd.h>
#include <tebako-dirent.h>
#include <tebako-dfs.h>

using namespace std;

sync_tebako_dstable sync_tebako_dstable::dstable;

uintptr_t sync_tebako_dstable::opendir(int vfd, size_t& size) noexcept {
	uintptr_t ret = 0;;
	int err = ENOTDIR;
	try {
		auto ds = make_shared<tebako_ds>(vfd);
		if (ds == NULL) {
			err = ENOMEM;
		}
		else {
			err = ds->load_cache(0, true);
			if (err == DWARFS_IO_CONTINUE) {
				ret = reinterpret_cast<uintptr_t>(ds.get());
				(**wlock())[ret] = ds;
				size = ds->dir_size;
			}
			else {
				ret = 0;
			}
		}
	}
	catch (bad_alloc&) {
		if (ret > 0) {
			closedir(ret);
			ret = 0;
		}
		err = ENOMEM;
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

int sync_tebako_dstable::closedir(uintptr_t dirp) noexcept {
	int ret = DWARFS_INVALID_FD;
	auto p_dstable = *wlock();
	auto p_ds = p_dstable->find(dirp);
	if (p_ds != p_dstable->end()) {
		ret = sync_tebako_fdtable::fdtable.close(p_ds->second->vfd);
		p_dstable->erase(dirp);
	}
	return ret;
}

void sync_tebako_dstable::close_all(void) noexcept {
	auto p_dstable = *wlock();
	for (auto it = p_dstable->begin(); it != p_dstable->end(); ++it) {
		sync_tebako_fdtable::fdtable.close(it->second->vfd);

	}
	p_dstable->clear();
}

long sync_tebako_dstable::telldir(uintptr_t dirp) noexcept {
	long ret = DWARFS_INVALID_FD;
	auto p_dstable = *rlock();
	auto p_ds = p_dstable->find(dirp);
	if (p_ds != p_dstable->end()) {
		ret = p_ds->second->dir_position;
	}
	return ret;
}

int sync_tebako_dstable::seekdir(uintptr_t dirp, long pos) noexcept {
	long ret = DWARFS_INVALID_FD;
	auto p_dstable = *rlock();
	auto p_ds = p_dstable->find(dirp);
	if (p_ds != p_dstable->end()) {
		p_ds->second->dir_position = pos;
		ret = DWARFS_IO_CONTINUE;
	}
	return ret;
}

long sync_tebako_dstable::dirfd(uintptr_t dirp) noexcept {
	long ret = DWARFS_INVALID_FD;
	auto p_dstable = *rlock();
	auto p_ds = p_dstable->find(dirp);
	if (p_ds != p_dstable->end()) {
		ret = p_ds->second->vfd;
	}
	return ret;
}

int sync_tebako_dstable::readdir(uintptr_t  dirp, struct dirent*& entry) noexcept {
	int ret = DWARFS_INVALID_FD;
	entry = NULL;
	auto p_dstable = *rlock();
	auto p_ds = p_dstable->find(dirp);
	if (p_ds != p_dstable->end()) {
		if (p_ds->second->dir_position < 0) {      // Should not happen ever
			ret = DWARFS_IO_ERROR;
			TEBAKO_SET_LAST_ERROR(EBADF);
		}
		else {
			if (p_ds->second->dir_position >= p_ds->second->dir_size) {
				ret = DWARFS_IO_CONTINUE;
			}
			else {
				if (p_ds->second->dir_position > p_ds->second->cache_start + p_ds->second->cache_size - 1 ||
					p_ds->second->dir_position < p_ds->second->cache_start) {
					try {
						ret = p_ds->second->load_cache(p_ds->second->dir_position, false);
						if (ret == DWARFS_IO_CONTINUE) {
							entry = &p_ds->second->cache[p_ds->second->dir_position++ - p_ds->second->cache_start].e;
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
					entry = &p_ds->second->cache[p_ds->second->dir_position++ - p_ds->second->cache_start].e;
					ret = DWARFS_IO_CONTINUE;
				}
			}
		}
	}
	return ret;
}

int tebako_ds::load_cache(int new_cache_start, bool set_pos) noexcept {
	int ret = sync_tebako_fdtable::fdtable.readdir(vfd, cache, new_cache_start,
			                                       TEBAKO_DIR_CACHE_SIZE, cache_size,
												   dir_size);

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
