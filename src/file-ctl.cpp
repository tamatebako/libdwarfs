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


 /*
 * tebaco_access
 * tebaco_stat
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/
 */

int tebako_access(const char* path, int amode) {
	int ret = -1;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
		ret = p_path ? dwarfs_access(p_path, amode, getuid(), getgid()) :
			access(path, amode);
	}
	return ret;
}

int tebako_stat(const char* path, struct stat* buf) {
	int ret = -1;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
		ret = p_path ? dwarfs_stat(p_path, buf) : stat(path, buf);
	}
	return ret;
}

int tebako_lstat(const char* path, struct stat* buf) {
	int ret = -1;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
		ret = p_path ? dwarfs_lstat(p_path, buf) : lstat(path, buf);
	}
	return ret;
}

ssize_t tebako_readlink(const char* path, char* buf, size_t bufsize) {
	ssize_t ret = -1;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
		if (p_path) {
			std::string lnk;
			ret = dwarfs_readlink(p_path, lnk);
			if (ret >= 0) {
				strncpy(buf, lnk.c_str(), bufsize);
				ret = std::min(lnk.length(), bufsize);
			}
		}
		else {
			ret = ::readlink(path, buf, bufsize);
		}
	}
	return ret;
}

#ifdef TEBAKO_HAS_GETATTRLIST
extern "C" int tebako_getattrlist (const char* path, struct attrlist * attrList, void * attrBuf,  size_t attrBufSize, unsigned long options) {
	int ret =  DWARFS_IO_ERROR;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(EFAULT);
	}
	else {
		const char* p_path = to_tebako_path(t_path, path);
        if (path) {
		    TEBAKO_SET_LAST_ERROR(ENOTSUP);
		}
		else {
			ret = ::getattrlist(path, attrList, attrBuf, attrBufSize, options);
		}
	}
	return ret;
}
#endif
