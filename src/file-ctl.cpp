/**
 *
 *  Copyright (c) 2021-2022, [Ribose Inc](https://www.ribose.com).
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
#include <tebako-pch-pp.h>
#include <tebako-common.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>

int tebako_access(const char* path, int amode) {
	int ret = -1;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		std::string lnk;
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
		if (p_path) {
			ret = dwarfs_access(p_path, amode, getuid(), getgid(), lnk);
			if (ret == DWARFS_S_LINK_OUTSIDE) ret = ::access(lnk.c_str(), amode);
		}
		else {
			ret = ::access(path, amode);
		}
	}
	return ret;
}

#ifdef TEBAKO_HAS_LSTAT
int tebako_lstat(const char* path, struct stat* buf) {
	int ret = -1;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
		ret = p_path ? dwarfs_lstat(p_path, buf) : ::lstat(path, buf);
	}
	return ret;
}
#endif

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

int tebako_stat(const char* path, struct stat* buf) {
	int ret = -1;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		std::string lnk;
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
		if (p_path) {
			ret = dwarfs_stat(p_path, buf, lnk);
			if (ret == DWARFS_S_LINK_OUTSIDE) ret = ::stat(lnk.c_str(), buf);
		}
		else {
			ret = ::stat(path, buf);
		}
	}
	return ret;
}

#ifdef TEBAKO_HAS_GETATTRLIST
int tebako_getattrlist (const char* path, struct attrlist * attrList, void * attrBuf,  size_t attrBufSize, unsigned long options) {
	int ret =  DWARFS_IO_ERROR;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(EFAULT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
        if (p_path) {
		    TEBAKO_SET_LAST_ERROR(ENOTSUP);
		}
		else {
			ret = ::getattrlist(path, attrList, attrBuf, attrBufSize, options);
		}
	}
	return ret;
}
#endif

extern "C" int within_tebako_memfs(const char* path) {
	return is_tebako_path(path) ? -1 : 0;
}
