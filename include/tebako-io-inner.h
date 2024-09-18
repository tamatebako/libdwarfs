/**
 *
 * Copyright (c) 2021-2024 [Ribose Inc](https://www.ribose.com).
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
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

const int DWARFS_IO_CONTINUE = 0;
// DWARFS_IO_ERROR is a real error
const int DWARFS_IO_ERROR = -1;
// DWARFS_INVALID_FD is a suggestion that given file descriptor point to *real*
// fs
const int DWARFS_INVALID_FD = -2;
// DWARFS_S_LINK_OUTSIDE indicates a soft link from memfs towards an entity
// outside memfs
const int DWARFS_S_LINK_OUTSIDE = -3;
// Any symlink or mount point
const int DWARFS_S_LINK_ABSOLUTE = -4;
const int DWARFS_S_LINK_RELATIVE = -5;

// ... just to keep conditions like if (x & O_BINARY) ubiqiotous
//     and avoid conditional compilation

#ifndef O_BINARY
#define O_BINARY 0x0
#endif

#ifndef O_DIRECTORY
#define O_DIRECTORY 0x0
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0x0
#endif

namespace tebako {
#ifdef _WIN32
struct tebako_dirent;
#else
union tebako_dirent;
#endif
}  // namespace tebako

int dwarfs_access(const std::string&, int amode, uid_t uid, gid_t gid, std::string& lnk) noexcept;
int dwarfs_lstat(const std::string&, struct stat* buf, std::string& lnk) noexcept;
int dwarfs_readlink(const std::string& path, std::string& link, std::string& lnk) noexcept;
int dwarfs_stat(const std::string& path, struct stat* buf, std::string& lnk, bool follow) noexcept;

int dwarfs_inode_access(uint32_t inode, int amode, uid_t uid, gid_t gid) noexcept;
int dwarfs_inode_relative_stat(uint32_t inode, const std::string& path, struct stat* buf, std::string& lnk, bool follow) noexcept;
ssize_t dwarfs_inode_read(uint32_t inode, void* buf, size_t size, off_t offset) noexcept;
int dwarfs_inode_readdir(uint32_t inode,
                         tebako::tebako_dirent* cache,
                         off_t cache_start,
                         size_t buffer_size,
                         size_t& cache_size,
                         size_t& dir_size) noexcept;
