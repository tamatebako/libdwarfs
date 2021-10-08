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

class tebako_fd {
public:
	dwarfs::inode_view node;
	struct stat st;
	uint64_t pos;
	std::string filename;
	void* payload;
};

class tebako_fdtable : public std::map<int, std::shared_ptr<tebako_fd>> {
public:
	int something;
};

static folly::Synchronized<tebako_fdtable*> fdtable{ new tebako_fdtable };



int dwarfs_open(const char *path, int flags,  ...)
{
	return -1; //  squash_open_inner(fs, path, 1);
}


/* int dwarfs_open_inner(sqfs* fs, const char* path, short follow_link)
{
	sqfs_err error;
	struct squash_file *file = calloc(1, sizeof(struct squash_file));
	short found;
	int fd;
	size_t nr;
	int *handle;

	// try locating the file and fetching its stat
	if (NULL == file)
	{
		errno = ENOMEM;
		return -1;
	}
	error = sqfs_inode_get(fs, &file->node, sqfs_inode_root(fs));
	if (SQFS_OK != error)
	{
		goto failure;
	}
	error = sqfs_lookup_path_inner(fs, &file->node, path, &found, follow_link);
	if (SQFS_OK != error)
	{
		goto failure;
	}
	file->filename = strdup(path);

	if (!found)
	{
		errno = ENOENT;
		goto failure;
	}
	error = sqfs_stat(fs, &file->node, &file->st);
	if (SQFS_OK != error)
	{
		goto failure;
	}
	file->fs = fs;
	file->pos = 0;

	// get a dummy fd from the system
	fd = dup(0);
	if (-1 == fd) {
		goto failure;
	}
	// make sure that our global fd table is large enough
	nr = fd + 1;

	MUTEX_LOCK(&squash_global_mutex);
	if (squash_global_fdtable.nr < nr)
	{
		// we secretly extend the requested size
		// in order to minimize the number of realloc calls
		nr *= 10;
		squash_global_fdtable.fds = realloc(squash_global_fdtable.fds,
						    nr * sizeof(struct squash_file *));
		if (NULL == squash_global_fdtable.fds)
		{
			errno = ENOMEM;
			goto failure;
		}
		memset(squash_global_fdtable.fds + squash_global_fdtable.nr,
		       0,
		       (nr - squash_global_fdtable.nr) * sizeof(struct squash_file *));
		squash_global_fdtable.nr = nr;
	}
	MUTEX_UNLOCK(&squash_global_mutex);

	// construct a handle (mainly) for win32
	handle = (int *)malloc(sizeof(int));
	if (NULL == handle) {
		errno = ENOMEM;
		goto failure;
	}
	*handle = fd;
	file->payload = (void *)handle;

	// insert the fd into the global fd table
	file->fd = fd;
	MUTEX_LOCK(&squash_global_mutex);
	squash_global_fdtable.fds[fd] = file;
        if (squash_global_fdtable.end < fd + 1) {
        	squash_global_fdtable.end = fd + 1;
        }
	MUTEX_UNLOCK(&squash_global_mutex);
	return fd;

failure:
	if (!errno) {
		errno = ENOENT;
	}
	free(file);
	return -1;
}



int dwarfs_close(int vfd)
{
	int ret;
	struct squash_file *file;

        if (!SQUASH_VALID_VFD(vfd)) {
                errno = EBADF;
                return -1;
        }
        ret = close(vfd);
	if (-1 == ret) {
		return -1;
	}
        MUTEX_LOCK(&squash_global_mutex);
        if (S_ISDIR(squash_global_fdtable.fds[vfd]->st.st_mode)) {
                SQUASH_DIR *dir = (SQUASH_DIR *) (squash_global_fdtable.fds[vfd]->payload);
                free(dir);
        } else {
                int *handle = (int *) (squash_global_fdtable.fds[vfd]->payload);
                free(handle);
        }

        file = squash_global_fdtable.fds[vfd];
        free(file->filename);
        free(file);

        squash_global_fdtable.fds[vfd] = NULL;
        if (vfd + 1 == squash_global_fdtable.end) {
                while (vfd >= 0 && NULL == squash_global_fdtable.fds[vfd]) {
                        vfd -= 1;
                }
                squash_global_fdtable.end = vfd + 1;
        } else {
                assert(squash_global_fdtable.end > vfd + 1);
        }
        MUTEX_UNLOCK(&squash_global_mutex);
        return 0;
}

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