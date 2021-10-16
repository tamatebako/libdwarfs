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

struct tebako_fd {
	struct stat st;
	uint64_t pos;
	string filename;
	int* handle;

	tebako_fd(const char* p) : filename(p), pos(0), handle(NULL) {	}
	~tebako_fd() { 
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
	
	int open(const char* path, int flags)  noexcept;
	int openat(int vfd, const char* path, int flags) noexcept;
	int close(int vfd) noexcept;
	void close_all(void) noexcept;
	int fstat(int vfd, struct stat* st) noexcept;
	ssize_t read(int vfd, void* buf, size_t nbyte) noexcept;
	ssize_t pread(int vfd, void* buf, size_t nbyte, off_t offset) noexcept;
	int readdir(int vfd, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) noexcept;
	ssize_t readv(int vfd, const struct iovec* iov, int iovcnt) noexcept;
	off_t lseek(int vfd, off_t offset, int whence) noexcept;

	static sync_tebako_fdtable fdtable;
};

sync_tebako_fdtable sync_tebako_fdtable::fdtable;

int dwarfs_open(const char* path, int flags)  noexcept {
	return sync_tebako_fdtable::fdtable.open(path, flags);
}

int dwarfs_openat(int vfd, const char* path, int flags) noexcept {
	return sync_tebako_fdtable::fdtable.openat(vfd, path, flags);
}

void dwarfs_fd_close_all(void)  noexcept {
	sync_tebako_fdtable::fdtable.close_all();
}

int dwarfs_close(int vfd)  noexcept {
	return sync_tebako_fdtable::fdtable.close(vfd);
}

int dwarfs_fstat(int vfd, struct stat* buf) noexcept {
	return sync_tebako_fdtable::fdtable.fstat(vfd, buf);
}

int dwarfs_fd_readdir(int vfd, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) noexcept {
	return sync_tebako_fdtable::fdtable.readdir(vfd, cache, cache_start, buffer_size, cache_size, dir_size);
}

ssize_t dwarfs_read(int vfd, void* buf, size_t nbyte) noexcept {
	return sync_tebako_fdtable::fdtable.read(vfd, buf, nbyte);
}

ssize_t dwarfs_pread(int vfd, void* buf, size_t nbyte, off_t offset) noexcept {
	return sync_tebako_fdtable::fdtable.pread(vfd, buf, nbyte, offset);
}

ssize_t dwarfs_readv(int vfd, const struct iovec* iov, int iovcnt) noexcept {
	return sync_tebako_fdtable::fdtable.readv(vfd, iov, iovcnt);
}

off_t dwarfs_lseek(int vfd, off_t offset, int whence)  noexcept {
	return sync_tebako_fdtable::fdtable.lseek(vfd, offset, whence);
}


int sync_tebako_fdtable::open(const char* path, int flags)  noexcept {
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
							(**wlock())[ret] = fd;
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

int sync_tebako_fdtable::openat(int vfd, const char* path, int flags) noexcept {
	struct stat stfd;
	int ret = fstat(vfd, &stfd);
	if (ret == DWARFS_IO_CONTINUE) {
		ret = DWARFS_IO_ERROR;
		if (!S_ISDIR(stfd.st_mode)) {
			// [ENOTDIR] ... or O_DIRECTORY was specified and the path argument resolves to a non - directory file.
			TEBAKO_SET_LAST_ERROR(ENOTDIR);
		}
		else {
			if (flags & (O_RDWR | O_WRONLY | O_TRUNC)) {
				//	[EROFS] The named file resides on a read - only file system and either O_WRONLY, O_RDWR, O_CREAT(if the file does not exist), or O_TRUNC is set in the oflag argument.
				TEBAKO_SET_LAST_ERROR(EROFS);
			}
			else {
				//  .... 
				//  If the access mode of the open file description associated with the file descriptor is not O_SEARCH, the function shall check whether directory searches are 
				//  If the access mode is O_SEARCH, the function shall not perform the check.
				//	.... 
				//	However, Linux does not support O_SEARCH (
				//  So, We will assume that it is not set 
				ret = dwarfs_inode_access(stfd.st_ino, X_OK, getuid(), getgid());
				if (ret == DWARFS_IO_CONTINUE) {
					try {
						auto fd = make_shared<tebako_fd>(path);
						if (dwarfs_inode_relative_stat(stfd.st_ino, path, &fd->st) == 0) {
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
										(**wlock())[ret] = fd;
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
				else {
					TEBAKO_SET_LAST_ERROR(EACCES);
				}
			}
		}

	}
	return ret;
}

int sync_tebako_fdtable::close(int vfd) noexcept {
	return ((**wlock()).erase(vfd) > 0) ? DWARFS_IO_CONTINUE : DWARFS_INVALID_FD;
}

void sync_tebako_fdtable::close_all(void) noexcept {
	(*wlock())->clear();
}

int sync_tebako_fdtable::fstat(int vfd, struct stat* st) noexcept {
	int ret = DWARFS_INVALID_FD;
	auto p_fdtable = *rlock();
	auto p_fd = p_fdtable->find(vfd);
	if (p_fd != p_fdtable->end()) {
		memcpy(st, &p_fd->second->st, sizeof(struct stat));
		ret = DWARFS_IO_CONTINUE;
	}
	return ret;
}

ssize_t sync_tebako_fdtable::read(int vfd, void* buf, size_t nbyte) noexcept {
	int ret = DWARFS_INVALID_FD;
	auto p_fdtable = *rlock();
	auto p_fd = p_fdtable->find(vfd);
	if (p_fd != p_fdtable->end()) {
		ret = dwarfs_inode_read(p_fd->second->st.st_ino, buf, nbyte, p_fd->second->pos);
		if (ret > 0) {
			p_fd->second->pos += ret;
		}
	}
	return ret;
}

ssize_t sync_tebako_fdtable::pread(int vfd, void* buf, size_t nbyte, off_t offset) noexcept {
	auto p_fdtable = *rlock();
	auto p_fd = p_fdtable->find(vfd);
	return (p_fd != p_fdtable->end()) ? dwarfs_inode_read(p_fd->second->st.st_ino, buf, nbyte, offset) : DWARFS_INVALID_FD;
}

int sync_tebako_fdtable::readdir(int vfd, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) noexcept {
	auto p_fdtable = *rlock();
	auto p_fd = p_fdtable->find(vfd);
	return (p_fd != p_fdtable->end()) ? dwarfs_inode_readdir(p_fd->second->st.st_ino, cache, cache_start, buffer_size, cache_size, dir_size) : DWARFS_INVALID_FD;
}

ssize_t sync_tebako_fdtable::readv(int vfd, const struct iovec* iov, int iovcnt) noexcept {
	uint32_t ino;
	off_t pos;
	ssize_t ret = DWARFS_INVALID_FD;
	auto p_fdtable = *rlock();
	auto p_fd = p_fdtable->find(vfd);
	if (p_fd != p_fdtable->end()) {
		ret = 0;
		for (int i = 0; i < iovcnt; ++i) {
			ssize_t ssize = dwarfs_inode_read(p_fd->second->st.st_ino, iov[i].iov_base, iov[i].iov_len, p_fd->second->pos);
			if (ssize > 0) {
				p_fd->second->pos += ssize;
				ret += ssize;
			}
			else {
				if (ssize < 0) {
					ret = DWARFS_IO_ERROR;
				}
				break;
			}
		}
	}
	return ret;
}

off_t sync_tebako_fdtable::lseek(int vfd, off_t offset, int whence)  noexcept {
	off_t pos;
	ssize_t ret = DWARFS_INVALID_FD;
	auto p_fdtable = *this->rlock();
	auto p_fd = p_fdtable->find(vfd);
	if (p_fd != p_fdtable->end()) {
		switch (whence) {
		case SEEK_SET:
			ret = pos = offset;
			break;
		case SEEK_CUR:
			ret = pos = p_fd->second->pos + offset;
			break;
		case SEEK_END:
			ret = pos = p_fd->second->st.st_size + offset;
			break;
		default:
			// [EINVAL] The whence argument is not a proper value, or the resulting file offset would be negative for a regular file, block special file, or directory.
			TEBAKO_SET_LAST_ERROR(EINVAL);
			ret = DWARFS_IO_ERROR;
			break;
		}
		if (pos < 0) {
			// [EOVERFLOW] The resulting file offset would be a value which cannot be represented correctly in an object of type off_t.
			TEBAKO_SET_LAST_ERROR((offset < 0 ? EINVAL : EOVERFLOW));
			ret = DWARFS_IO_ERROR;
		}
		else {
			p_fd->second->pos = pos;
		}
	}
	return ret;
}






