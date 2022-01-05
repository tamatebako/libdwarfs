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

#include <tebako-pch.h>
#include <tebako-common.h>
#include <tebako-pch-pp.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-fd.h>

/*
*  tebako_open
*  tebako_close
*  tebako_lseek
*  tebako_read
*  tebako_readv
*  tebako_fstat
*
*  https://pubs.opengroup.org/onlinepubs/9699919799/
*/

extern "C" int tebako_open(int nargs, const char* path, int flags, ...)
{
	int ret = -1;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);

		if (p_path) {
			return sync_tebako_fdtable::fdtable.open(p_path, flags);
		}
		else {
			if (nargs == 2) {
				ret = open(path, flags);
			}
			else {
				va_list args;
				mode_t mode;
				va_start(args, flags);
// https://stackoverflow.com/questions/28054194/char-type-in-va-arg
// second argument to 'va_arg' is of promotable type 'mode_t' (aka 'unsigned short');
// this va_arg has undefined behavior because arguments will be promoted to 'int'
				mode = (mode_t)va_arg(args, int);
				va_end(args);
				ret = ::open(path, flags, mode);
			}
		}
	}
	return ret;
}

extern "C" int tebako_openat(int nargs, int vfd, const char* path, int flags, ...) {
	int ret = -1;
	va_list args;
	mode_t mode;
	//	The openat() function shall be equivalent to the open() function except in the case where path specifies a relative path.
	//  In this case the file to be opened is determined relative to the directory associated with the file descriptor fd instead of the current working directory.
    //  ...
	//	If openat() is passed the special value AT_FDCWD in the fd parameter, the current working directory shall be used and the behavior shall be identical to a call to open().
	try {
		std::filesystem::path std_path(path);
		if (std_path.is_relative() && vfd != AT_FDCWD) {
			ret = sync_tebako_fdtable::fdtable.openat(vfd, path, flags);
			if (ret == DWARFS_INVALID_FD) {
				if (nargs == 3) {
					ret = ::openat(vfd, path, flags);
				}
				else {
					va_start(args, flags);
					mode = (mode_t)va_arg(args, int);
					va_end(args);
					ret = ::openat(vfd, path, flags, mode);
				}
			}
		}
		else {
			if (nargs == 3) {
				ret = tebako_open(2, path, flags);
			}
			else {
				va_start(args, flags);
				mode = (mode_t)va_arg(args, int);
				va_end(args);
				ret = tebako_open(3, path, flags, mode);
			}

		}
	}
	catch (...) {
		ret = -1;
		TEBAKO_SET_LAST_ERROR(ENOMEM);
	}
	return ret;
}

extern "C" off_t tebako_lseek(int vfd, off_t offset, int whence) {
	off_t ret = sync_tebako_fdtable::fdtable.lseek(vfd, offset, whence);
	return (ret == DWARFS_INVALID_FD) ? ::lseek(vfd, offset, whence) : ret;
}

extern "C" ssize_t tebako_read(int vfd, void* buf, size_t nbyte) {
	ssize_t ret = sync_tebako_fdtable::fdtable.read(vfd, buf, nbyte);
	return (ret == DWARFS_INVALID_FD) ? ::read(vfd, buf, nbyte) : ret;
}

extern "C" ssize_t tebako_readv(int vfd, const struct iovec* iov, int iovcnt) {
	ssize_t ret = sync_tebako_fdtable::fdtable.readv(vfd, iov, iovcnt);
	return (ret == DWARFS_INVALID_FD) ? ::readv(vfd, iov, iovcnt) : ret;
}

extern "C" ssize_t tebako_pread(int vfd, void* buf, size_t nbyte, off_t offset) {
	ssize_t ret = sync_tebako_fdtable::fdtable.pread(vfd, buf, nbyte, offset);
	return (ret == DWARFS_INVALID_FD) ? ::pread(vfd, buf, nbyte, offset) : ret;
}

extern "C" int tebako_close(int vfd) {
	int ret = sync_tebako_fdtable::fdtable.close(vfd);
	return (ret == DWARFS_INVALID_FD) ? ::close(vfd) : ret;
}

extern "C" int tebako_fstat(int vfd, struct stat* buf) {
	int ret = sync_tebako_fdtable::fdtable.fstat(vfd, buf);
	return (ret == DWARFS_INVALID_FD) ? ::fstat(vfd, buf) : ret;
}

extern "C" int tebako_fstatat(int vfd, const char* path, struct stat* buf, int flag) {
	int ret = -1;
	try {
		std::filesystem::path std_path(path);
		if (std_path.is_absolute() || vfd == AT_FDCWD) {
			ret = (flag== AT_SYMLINK_NOFOLLOW) ? tebako_lstat(path, buf) : tebako_stat(path, buf);
		}
		else {
			ret = sync_tebako_fdtable::fdtable.fstatat(vfd, path, buf);
			if (ret == DWARFS_INVALID_FD) {
				ret = ::fstatat(vfd, path, buf, flag);
			}
		}
	}
	catch (...) {
		ret = -1;
		TEBAKO_SET_LAST_ERROR(ENOMEM);
	}
	return ret;
}

#if TEBAKO_HAS_FGETATTRLIST
extern "C" int tebako_fgetattrlist (int fd, struct attrlist * attrList, void * attrBuf, size_t attrBufSize, unsigned long options) {
	struct stat stfd;
	int ret = fstat(vfd, &stfd);
	if (ret == DWARFS_IO_ERROR) {
		ret = ::fgetattrlist (fd, attrList, attrBuf, attrBufSize, options);
	}
	else {
		ret = -1;
		TEBAKO_SET_LAST_ERROR(ENOTSUP);
	}
	return ret;
}
#endif
