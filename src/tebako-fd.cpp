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
#include "tebako-io-inner.h"
#include <tebako-dfs.h>

using namespace std;

struct tebako_fd {
	struct stat st;
	uint64_t pos;
	string filename;
	int* handle;

	struct dirent* dtbl;
	long           dpos;

	tebako_fd(const char* p) : filename(p), pos(0), handle(NULL), dtbl(NULL), dpos(-1) {	}
	~tebako_fd() { 
		if (dtbl) {
			delete dtbl;
		}
		dtbl = NULL;
		dpos = -1;

		if (handle) {
			::close(*handle);
			delete(handle);
		}
		handle = NULL; 
	}
};

typedef map<int, shared_ptr<tebako_fd>> tebako_fdtable;

class sync_tebako_fdtable : public folly::Synchronized<tebako_fdtable*> {
public:
	sync_tebako_fdtable(void): folly::Synchronized<tebako_fdtable*>(new tebako_fdtable) { }

	void close_all(void) {
		auto p_fdtable = *this->wlock();
		p_fdtable->clear();
	}

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

	ssize_t sync_op_start(int vfd, uint32_t& ino, int*& handle) {
		ssize_t ret = DWARFS_INVALID_FD;
		auto p_fdtable = *this->rlock();
		auto p_fd = p_fdtable->find(vfd);
		if (p_fd != p_fdtable->end()) {
			ino = p_fd->second->st.st_ino;
			handle = p_fd->second->handle;
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}

	ssize_t sync_op_start(int vfd, uint32_t& ino, off_t& pos) {
		ssize_t ret = DWARFS_INVALID_FD;
		auto p_fdtable = *this->rlock();
		auto p_fd = p_fdtable->find(vfd);
		if (p_fd != p_fdtable->end()) {
			ino = p_fd->second->st.st_ino;
			pos = p_fd->second->pos;
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}

	ssize_t sync_op_start2(int vfd, off_t& size, off_t& pos) {
		ssize_t ret = DWARFS_INVALID_FD;
		auto p_fdtable = *this->rlock();
		auto p_fd = p_fdtable->find(vfd);
		if (p_fd != p_fdtable->end()) {
			pos = p_fd->second->pos;
			size = p_fd->second->st.st_size;
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}

	ssize_t sync_pos_set(int vfd, off_t pos) {
		ssize_t ret = DWARFS_IO_ERROR;
		auto p_fdtable = *this->rlock();
		auto p_fd = p_fdtable->find(vfd);
		if (p_fd != p_fdtable->end()) {
			p_fd->second->pos = pos;
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}

	ssize_t sync_dtbl_set(int vfd, struct dirent* dtable) {
		ssize_t ret = DWARFS_IO_ERROR;
		auto p_fdtable = *this->rlock();
		auto p_fd = p_fdtable->find(vfd);
		if (p_fd != p_fdtable->end()) {
			p_fd->second->dtbl = dtable;
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}

	int sync_get_stat(int vfd, struct stat* buf) {
		int ret = DWARFS_INVALID_FD;
		auto p_fdtable = *this->rlock();
		auto p_fd = p_fdtable->find(vfd);
		if (p_fd != p_fdtable->end()) {
			memcpy(buf, &p_fd->second->st, sizeof(struct stat));
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}

	int sync_get_stat_fname(int vfd, string& fname) {
		int ret = DWARFS_INVALID_FD;
		auto p_fdtable = *this->rlock();
		auto p_fd = p_fdtable->find(vfd);
		if (p_fd != p_fdtable->end()) {
			fname = p_fd->second->filename;
			ret = DWARFS_IO_CONTINUE;
		}
		return ret;
	}

};

static sync_tebako_fdtable fdtable;


void dwarfs_close_all(void)  noexcept {
	fdtable.close_all();
}

// We do not set errno=ENOENT in th functions below since native function will be called in case of error either here or by the calling routine


int dwarfs_open(const char *path, int flags)  noexcept {
	int ret = DWARFS_IO_ERROR;
	if (flags & (O_RDWR | O_WRONLY | O_TRUNC)) {
		//	[EROFS] The named file resides on a read - only file system and either O_WRONLY, O_RDWR, O_CREAT(if the file does not exist), or O_TRUNC is set in the oflag argument.
		TEBAKO_SET_LAST_ERROR(EROFS);
	}
	else {
		try {
			auto fd = make_shared<tebako_fd>(path);
			if (dwarfs_stat(path, &fd->st) == 0) {
				if (!S_ISDIR(fd->st.st_mode) && (flags & O_DIRECTORY)) {
					// [ENOTDIR] ... or O_DIRECTORY was specified and the path argument resolves to a non - directory file.
					TEBAKO_SET_LAST_ERROR(ENOTDIR);
				}
				else {
					fd->handle = new int;
					if (fd->handle == NULL) {
						TEBAKO_SET_LAST_ERROR(ENOMEM);
					}
					else {
						// get a dummy fd from the system
						ret = dup(0);
						if (ret == DWARFS_IO_ERROR) {
					// [EMFILE]  All file descriptors available to the process are currently open.
							TEBAKO_SET_LAST_ERROR(EMFILE);        
						}
						else {
							// construct a handle (mainly) for win32
							*fd->handle = ret;
							(**fdtable.wlock())[ret] = fd;
						}
					}
				}
			}
			else {
			//	[ENOENT] O_CREAT is not set and a component of path does not name an existing file, or O_CREAT is set and a component of the path prefix of path does not name an existing file, or path points to an empty string.
			//	[EROFS] The named file resides on a read - only file system and either O_WRONLY, O_RDWR, O_CREAT(if the file does not exist), or O_TRUNC is set in the oflag argument.
				TEBAKO_SET_LAST_ERROR((flags & O_CREAT) ? EROFS : ENOENT);
			}
		}
		catch (bad_alloc&) {
			if (ret > 0) {
				close(ret);
				ret = DWARFS_IO_ERROR;
			}
			TEBAKO_SET_LAST_ERROR(ENOMEM);
		}
	}
	return ret;
}

int dwarfs_openat(int vfd, const char* path, int flags) noexcept  {
	string fname;
	int ret = fdtable.sync_get_stat_fname(vfd, fname);
	if (ret == DWARFS_IO_CONTINUE) {
		//  .... 
		//  If the access mode of the open file description associated with the file descriptor is not O_SEARCH, the function shall check whether directory searches are 
		//  If the access mode is O_SEARCH, the function shall not perform the check.
		//	.... 
		//	However, Linux does not support O_SERACH (
		//  So, We will assume that it is not set 
		std::filesystem::path pname(fname);
		pname.remove_filename();
		if (dwarfs_access(pname.c_str(), X_OK, getuid(), getgid()) == 0) {
			pname /= path;
			ret = dwarfs_open(pname.c_str(), flags);
		}
		else {
			TEBAKO_SET_LAST_ERROR(EACCES);
			ret = DWARFS_IO_ERROR;
		}
	}
	return ret;
}


int dwarfs_close(int vfd)  noexcept {
	return ((**fdtable.wlock()).erase(vfd) > 0) ? DWARFS_IO_CONTINUE : DWARFS_INVALID_FD;
}

off_t dwarfs_lseek(int vfd, off_t offset, int whence)  noexcept {
	off_t pos;
	off_t size;
	ssize_t ret = fdtable.sync_op_start2(vfd, size, pos);
	if (ret == DWARFS_IO_CONTINUE) {
		switch (whence) {
		case SEEK_SET:
			ret = pos = offset;
			break;
		case SEEK_CUR:
			ret = pos = pos + offset;
			break;
		case SEEK_END:
			ret = pos = size + offset;
			break;
		default:
			// [EINVAL] The whence argument is not a proper value, or the resulting file offset would be negative for a regular file, block special file, or directory.
			TEBAKO_SET_LAST_ERROR(EINVAL);
			ret = DWARFS_IO_ERROR;
			break;
		}
		if (pos >= 0) {
			ssize_t r = fdtable.sync_pos_set(vfd, pos);
			if (r != DWARFS_IO_CONTINUE) {
				TEBAKO_SET_LAST_ERROR(EBADF);
				ret = DWARFS_IO_ERROR;
			}
		}
		else {
			// [EOVERFLOW] The resulting file offset would be a value which cannot be represented correctly in an object of type off_t.
			TEBAKO_SET_LAST_ERROR((offset < 0 ? EINVAL : EOVERFLOW));
			ret = DWARFS_IO_ERROR;
		}
	}
	return ret;
}

ssize_t dwarfs_read(int vfd, void* buf, size_t nbyte)   noexcept {
	uint32_t ino;
	off_t pos;
	ssize_t ret = fdtable.sync_op_start(vfd, ino, pos);
	if (ret == DWARFS_IO_CONTINUE) {
		ret = dwarfs_inode_read(ino, buf, nbyte, pos);
		if (ret > 0) {
			ssize_t r = fdtable.sync_pos_set(vfd, pos);
			if (r != DWARFS_IO_CONTINUE) {
				ret = r;
			}
		}
	}
	return ret;
}

ssize_t dwarfs_pread(int vfd, void* buf, size_t nbyte, off_t offset)  noexcept {
	uint32_t ino;
	ssize_t ret = fdtable.sync_op_start(vfd, ino);
	return (ret == DWARFS_IO_CONTINUE) ? dwarfs_inode_read(ino, buf, nbyte, offset) : ret;
}

ssize_t dwarfs_readv(int vfd, const struct iovec* iov, int iovcnt)  noexcept {
	uint32_t ino;
	off_t pos;
	ssize_t ret = fdtable.sync_op_start(vfd, ino, pos);
	if (ret == DWARFS_IO_CONTINUE) {
		for (int i = 0; i < iovcnt; ++i) {
			ssize_t ssize = dwarfs_inode_read(ino, iov[i].iov_base, iov[i].iov_len, pos);
			if (ssize > 0) {
				pos += ssize;
				ret += ssize;
			}
			else {
				if (ssize < 0) {
					ret = DWARFS_IO_ERROR;
				}
				break;
			}
		}
		ssize_t r = fdtable.sync_pos_set(vfd, pos);
		if (r != DWARFS_IO_CONTINUE) {
			ret = r;
		}
	}
	return ret;
}

int dwarfs_fstat(int vfd, struct stat* buf)  noexcept {
	return fdtable.sync_get_stat(vfd, buf);
}

int dwarfs_fdopendir(int vfd, DIR*& dhandle) noexcept {
	dhandle = NULL;
	uint32_t ino;
	int* handle;
	int ret = fdtable.sync_op_start(vfd, ino, handle);
	if (ret == DWARFS_IO_CONTINUE) {
		struct dirent* dtbl;
		ret = dwarfs_inode_opendir(ino, dtbl);
		if (ret == DWARFS_IO_CONTINUE) {
			dhandle = reinterpret_cast<DIR*>(handle);
			fdtable.sync_dtbl_set(vfd, dtbl);
		}
	}
	 
	return ret;
}