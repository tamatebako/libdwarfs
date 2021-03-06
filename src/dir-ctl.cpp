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
#include <tebako-io.h>
#include <tebako-io-inner.h>

/*
*   getcwd()
*   https://pubs.opengroup.org/onlinepubs/9699919799/
*/

extern "C" char* tebako_getcwd(char* buf, size_t size) {
		char _cwd[TEBAKO_PATH_LENGTH];
		const char* cwd = tebako_get_cwd(_cwd);
		size_t len = strlen(cwd);
		if (len) {
			if (!buf) {
				if (!size) {
					buf = strdup(cwd);
					if (!buf) {
						TEBAKO_SET_LAST_ERROR(ENOMEM);
					}
				}
				else {
					if (len > size-1) {
						TEBAKO_SET_LAST_ERROR(ERANGE);
						buf = NULL;
					}
					else {
						buf = (char*)malloc(size);
						if (!buf) {
							TEBAKO_SET_LAST_ERROR(ENOMEM);
						}
						else {
							strcpy(buf, cwd);
						}
					}
				}
			}
			else {
				if (!size) {
					TEBAKO_SET_LAST_ERROR(EINVAL);
					buf = NULL;
				}
				else if (len > size-1)
				{
					TEBAKO_SET_LAST_ERROR(ERANGE);
					buf = NULL;
				}
				else
				    strcpy(buf, cwd);
			}
			return buf;
		}
		else
   		  return ::getcwd(buf, size);
	}

/*
*   getwd()
*	LEGACY, DEPRECATED
*	https://pubs.opengroup.org/onlinepubs/009695299/functions/getwd.html
*/
#ifdef WITH_GETWD
extern "C"	char* tebako_getwd(char* buf) {
	char * ret = NULL;
	if (buf == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		if (is_tebako_cwd()) {
			tebako_get_cwd(buf);
			ret = buf;
		}
		else {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
			ret = ::getwd(buf);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
		}
	}
	return ret;
}
#endif

/*
* chdir()
* https://pubs.opengroup.org/onlinepubs/9699919799/
*
*/

extern "C"	int tebako_chdir(const char* path) {
	int ret = DWARFS_IO_ERROR;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);

		if (p_path) {
			struct stat st;
			ret = tebako_stat(p_path, &st);
			if (ret == 0) {
				if (S_ISDIR(st.st_mode)) {
					ret = tebako_set_cwd(p_path) ? 0 : -1;
					if (ret != 0) {
						TEBAKO_SET_LAST_ERROR(ENOMEM);
					}
				}
				else {
					ret = -1;
					TEBAKO_SET_LAST_ERROR(ENOTDIR);
				}
			}
		}
		else {
			ret = ::chdir(path);
			if (ret == 0) {
				ret = tebako_set_cwd(NULL) ? 0 : -1;
				if (ret != 0) {
					TEBAKO_SET_LAST_ERROR(ENOMEM);
				}
			}
		}
	}

	return ret;
}

/*
* mkdir()
* https://pubs.opengroup.org/onlinepubs/9699919799/
*
*/

extern "C"	int tebako_mkdir(const char* path, mode_t mode) {
	int ret = DWARFS_IO_ERROR;
	if (path == NULL) {
		TEBAKO_SET_LAST_ERROR(ENOENT);
	}
	else {
		if ((is_tebako_cwd() && path[0] != '/') || is_tebako_path(path)) {
			TEBAKO_SET_LAST_ERROR(EROFS);
		}
		else {
			ret = ::mkdir(path, mode);
		}
	}
	return ret;
}
