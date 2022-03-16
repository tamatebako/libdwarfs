/**
 *
 * Copyright (c) 2021-2022 [Ribose Inc](https://www.ribose.com).
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

union tebako_dirent;

struct tebako_fd {
	struct stat st;
	uint64_t pos;
	std::string filename;
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

typedef std::map<int, std::shared_ptr<tebako_fd>> tebako_fdtable;

class sync_tebako_fdtable : public folly::Synchronized<tebako_fdtable*> {
public:
	sync_tebako_fdtable(void) : folly::Synchronized<tebako_fdtable*>(new tebako_fdtable) { }

	int open(const char* path, int flags, std::string& lnk)  noexcept;
	int openat(int vfd, const char* path, int flags) noexcept;
	int close(int vfd) noexcept;
	void close_all(void) noexcept;
	int fstat(int vfd, struct stat* st) noexcept;
	ssize_t read(int vfd, void* buf, size_t nbyte) noexcept;
	ssize_t pread(int vfd, void* buf, size_t nbyte, off_t offset) noexcept;
	int readdir(int vfd, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) noexcept;
	ssize_t readv(int vfd, const struct iovec* iov, int iovcnt) noexcept;
	off_t lseek(int vfd, off_t offset, int whence) noexcept;
	int fstatat(int vfd, const char* path, struct stat* buf) noexcept;

	static sync_tebako_fdtable fdtable;
};





