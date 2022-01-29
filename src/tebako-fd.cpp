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
#include <tebako-dfs.h>

using namespace std;

sync_tebako_fdtable sync_tebako_fdtable::fdtable;

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
				//  If the access mode of the open file description associated with the file descriptor is not O_SEARCH, the function shall check whether directory searches are allowed
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

int sync_tebako_fdtable::fstatat(int vfd, const char* path, struct stat* st) noexcept {
	struct stat stfd;
	int ret = fstat(vfd, &stfd);
	if (ret == DWARFS_IO_CONTINUE) {
		ret = DWARFS_IO_ERROR;
		if (!S_ISDIR(stfd.st_mode)) {
			TEBAKO_SET_LAST_ERROR(ENOTDIR);
		}
		else {
			ret = dwarfs_inode_access(stfd.st_ino, X_OK, getuid(), getgid());
			if (ret == DWARFS_IO_CONTINUE) {
				ret = dwarfs_inode_relative_stat(stfd.st_ino, path, st);
			}
			else {
				TEBAKO_SET_LAST_ERROR(ENOENT);
			}
		}
	}
	return ret;
}
