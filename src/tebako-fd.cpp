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
	uint32_t inode_num;
	struct stat st;
	uint64_t pos;
	string filename;
	// [TODO] void* does not look good enough
	void* payload;

	tebako_fd(const char* p) : filename(p), pos(0), payload(NULL) {	}
	~tebako_fd() { if (payload) free(payload); payload = NULL; }
};

typedef map<int, shared_ptr<tebako_fd>> tebako_fdtable;

static folly::Synchronized<tebako_fdtable*> fdtable{ new tebako_fdtable };


int dwarfs_open(const char *path, int flags)
{
	int ret = -1;
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
					int* handle = (int*)malloc(sizeof(int));
					if (handle == NULL) {
						TEBAKO_SET_LAST_ERROR(ENOMEM);
					}
					else {
						// get a dummy fd from the system
						ret = dup(0);
						if (ret == -1) {
					// [EMFILE]  All file descriptors available to the process are currently open.
							TEBAKO_SET_LAST_ERROR(EMFILE);        
						}
						else {
							// construct a handle (mainly) for win32
							*handle = ret;
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
				ret = -1;
			}
			TEBAKO_SET_LAST_ERROR(ENOMEM);
		}
	}
	return ret;
}

int dwarfs_close(int vfd)
{
	// We do not set errno in this function since ::close will be called in case of error either here or in tebako_close
	int ret = DWARFS_INVALID_FD;
	if ((**fdtable.wlock()).erase(vfd) > 0) {
		ret = close(vfd);
	}
	return ret ;
}


off_t dwarfs_lseek(int vfd, off_t offset, int whence)
{
	// We do not set errno in this function if vfd is not found since ::lseek will be called in case of error either here or in tebako_close
	off_t ret = DWARFS_INVALID_FD;
	off_t pos = 0;
	auto p_fdtable = *fdtable.rlock();
	auto p_fd = p_fdtable->find(vfd);
	if (p_fd != p_fdtable->end()) {
		switch (whence) {
		case SEEK_SET:
			pos = offset;
			break;
		case SEEK_CUR:
			pos = p_fd->second->pos + offset;
			break;
		case SEEK_END:
			pos = p_fd->second->st.st_size + offset;
			break;
		default:
			// [EINVAL] The whence argument is not a proper value, or the resulting file offset would be negative for a regular file, block special file, or directory.
			TEBAKO_SET_LAST_ERROR(EINVAL);
			ret = -1;
			break;
		}
		if (pos >= 0) {
			ret = p_fd->second->pos = pos;
		}
		else {
			// [EOVERFLOW] The resulting file offset would be a value which cannot be represented correctly in an object of type off_t.
			TEBAKO_SET_LAST_ERROR((offset<0 ? EINVAL: EOVERFLOW));
			ret = -1;
		}
	}
		
	return ret;

}

ssize_t dwarfs_read(int vfd, void* buf, size_t nbyte)
{
	// We do not set errno in this function since ::read will be called in case of error either here or in tebako_close
	int ret = DWARFS_INVALID_FD;
	auto p_fdtable = *fdtable.rlock();
	auto p_fd = p_fdtable->find(vfd);
	if (p_fd != p_fdtable->end()) {
		ret = dwarfs_inode_read(p_fd->second->st.st_ino, buf, nbyte, p_fd->second->pos);
		if (ret > 0) {
			p_fd->second->pos += ret;
		}
	}
	return ret;

}


/*

ssize_t dwarfs_read(int vfd, void *buf, sqfs_off_t nbyte)
{
	sqfs_err error;
	struct squash_file *file;

	if (!SQUASH_VALID_VFD(vfd))
	{
		errno = EBADF;
		goto failure;
	}
	file = squash_global_fdtable.fds[vfd];

	error = sqfs_read_range(file->fs, &file->node, file->pos, &nbyte, buf);
	if (SQFS_OK != error)
	{
		goto failure;
	}
	file->pos += nbyte;
	return nbyte;
failure:
	if (!errno) {
		errno = EBADF;
	}
	return -1;
}



*/