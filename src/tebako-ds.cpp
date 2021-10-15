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

const size_t dir_cache_size = 50;

struct tebako_ds {
	struct dirent cache[dir_cache_size];
	size_t cache_load;
	long pos;
	long cache_start;
	int vfd;

	tebako_ds(int fd) : cache_load(0), cache_start(0), pos(-1), vfd(fd) {
	}

	int load_cache(int cpos) {
		return dwarfs_fd_readdir(vfd, cache, cpos, dir_cache_size, cache_load);
	}

};

typedef vector<shared_ptr<tebako_ds>> tebako_dstable;

class sync_tebako_dstable : public folly::Synchronized<tebako_dstable*> {
public:
	sync_tebako_dstable(void) : folly::Synchronized<tebako_dstable*>(new tebako_dstable) { }

	void close_all(void) {
		auto p_dstable = *this->wlock();
		p_dstable->clear();
	}

	struct tebako_ds* opendir(int vfd) {
			struct tebako_ds* ret = NULL;
			int err = ENOTDIR;
			try {
				auto ds = make_shared<tebako_ds>(tebako_ds(vfd));
				if (ds == NULL) {
					err = ENOMEM;
				}
				else {
					err = ds->load_cache(0);
					if (err == DWARFS_IO_CONTINUE) {
						ret = ds.get();
						auto p_dstable = *this->wlock();
						p_dstable->push_back(ds);
					}
					else {
						ret = NULL;
					}
				}
			}
			catch (...) {
				ret = NULL;
				err = EIO;
			}
			if (ret) {
				TEBAKO_SET_LAST_ERROR(err);
			}
			return ret;
		}
};
/*
	ssize_t sync_op_start(int vfd, uint32_t& ino) {
		ssize_t ret = DWARFS_INVALID_FD;
		auto p_fdtable = *this->rlock();
		auto p_fd = p_fdtable->find(vfd);
		if (p_fd != p_fdtable->end()) {
			ino = p_fd->second->st.st_ino;
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}
*/

static sync_tebako_dstable dstable;


void dwarfs_dir_close_all(void)  noexcept {
	dstable.close_all();
}


DIR* dwarfs_fdopendir(int vfd) noexcept {
	return reinterpret_cast<DIR*>(dstable.opendir(vfd));
}