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
				// get a dummy fd from the system
				ret = dup(0);
				if (ret == -1) {
					// [EMFILE]  All file descriptors available to the process are currently open.
					TEBAKO_SET_LAST_ERROR(EMFILE);        
				}
				else {
					int* handle = (int*)malloc(sizeof(int));
					if (handle == NULL) {
						TEBAKO_SET_LAST_ERROR(ENOMEM);
					}
					else {
						// construct a handle (mainly) for win32
						*handle = ret;
						(**fdtable.wlock())[ret] = fd;
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
			TEBAKO_SET_LAST_ERROR(ENOMEM);
		}
	}
	return ret;
}


int dwarfs_close(int vfd)
{
	// We do not set errno in this function since ::close will be called in case of error either here or in tebako_close
	int ret = -1;
	if ((**fdtable.wlock()).erase(vfd) > 0) {
		ret = close(vfd);
	}
	else {
		ret = DWARFS_INVALID_FD;
	}
	return ret ;
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

off_t dwarfs_lseek(int vfd, off_t offset, int whence)
{
	struct squash_file *file;
	if (!SQUASH_VALID_VFD(vfd))
	{
		errno = EBADF;
		return -1;
	}
	file = squash_global_fdtable.fds[vfd];
	if (SQUASH_SEEK_SET == whence)
	{
		file->pos = offset;
	}
	else if (SQUASH_SEEK_CUR == whence)
	{
		file->pos += offset;
	}
	else if (SQUASH_SEEK_END == whence)
	{
		assert(S_ISREG(file->node.base.mode));
		file->pos = file->node.xtra.reg.file_size;
	}
	return file->pos;
}

*/