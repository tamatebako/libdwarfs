/**
 *
 * Copyright (c) 2021, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of tebako (dwarfs-wr)
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

#pragma once

#include "tebako-pch.h"

const int DWARFS_IO_CONTINUE = 0;
const int DWARFS_IO_ERROR = -1;
const int DWARFS_INVALID_FD = -2;


/* The d_name field
 The dirent structure definition shown above is taken from the
 glibc headers, and shows the d_name field with a fixed size.

 Warning: applications should avoid any dependence on the size of
 the d_name field.POSIX defines it as char d_name[], a character
 array of unspecified size, with at most NAME_MAX characters
 preceding the terminating null byte('\0').

 POSIX.1 explicitly notes that this field should not be used as an
 lvalue.The standard also notes that the use of sizeof(d_name)
 is incorrect; use strlen(d_name) instead.  (On some systems, this
    field is defined as char d_name[1]!)  By implication, the use
    sizeof(struct dirent) to capture the size of the record including
    the size of d_name is also incorrect.

    Note that while the call

    fpathconf(fd, _PC_NAME_MAX)

    returns the value 255 for most filesystems, on some filesystems
    (e.g., CIFS, Windows SMB servers), the null - terminated filename
    that is(correctly) returned in d_name can actually exceed this
    size.In such cases, the d_reclen field will contain a value
    that exceeds the size of the glibc dirent structure shown above.
*/

typedef union tebako_dirent {
    struct dirent e;
    char padding[TEBAKO_PATH_LENGTH + 1 + (sizeof(struct dirent) - 256 * sizeof(char))];
} tebako_dirent;



int dwarfs_access(const char* path, int amode, uid_t uid, gid_t gid) noexcept;
int dwarfs_stat(const char* path, struct stat* buf) noexcept ;
int dwarfs_open(const char* path, int mode)  noexcept;
int dwarfs_openat(int vfd, const char* path, int flags) noexcept;
off_t dwarfs_lseek(int vfd, off_t offset, int whence)  noexcept;
ssize_t dwarfs_read(int vfd, void* buf, size_t nbyte)  noexcept;
ssize_t dwarfs_readv(int vfd, const struct iovec* iov, int iovcnt)  noexcept;
ssize_t dwarfs_pread(int vfd, void* buf, size_t nbyte, off_t offset)  noexcept;
int   dwarfs_fstat(int vfd, struct stat* buf)  noexcept;
int dwarfs_close(int vfd)  noexcept;

DIR* dwarfs_fdopendir(int vfd) noexcept;
int dwarfs_closedir(DIR* dirp) noexcept;
long dwarfs_telldir(DIR* dirp) noexcept;
int dwarfs_seekdir(DIR* dirp, long pos) noexcept;
int dwarfs_dirfd(DIR* dirp) noexcept;
int dwarfs_readdir(DIR* dirp, struct dirent*& entry);

int dwarfs_inode_access(uint32_t inode, int amode, uid_t uid, gid_t gid)  noexcept;
int dwarfs_inode_relative_stat(uint32_t inode, const char* path, struct stat* buf) noexcept;
ssize_t dwarfs_inode_read(uint32_t inode, void* buf, size_t size, off_t offset) noexcept;
int dwarfs_fd_readdir(int vfd, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) noexcept;
int dwarfs_inode_readdir(uint32_t inode, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) noexcept;

void dwarfs_fd_close_all(void)  noexcept;
void dwarfs_dir_close_all(void)  noexcept;


