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

#pragma once

#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <version.h>

#define _TEBAKO_PP_NARG(...) \
    _TEBAKO_PP_NARG_(__VA_ARGS__,_TEBAKO_PP_RSEQ_N())
#define _TEBAKO_PP_NARG_(...) \
    _TEBAKO_PP_ARG_N(__VA_ARGS__)
#define _TEBAKO_PP_ARG_N( \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
    _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
    _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
    _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
    _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
    _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
    _61,_62,_63,  N, ...) N
#define _TEBAKO_PP_RSEQ_N() \
    63,62,61,60,                   \
    59,58,57,56,55,54,53,52,51,50, \
    49,48,47,46,45,44,43,42,41,40, \
    39,38,37,36,35,34,33,32,31,30, \
    29,28,27,26,25,24,23,22,21,20, \
    19,18,17,16,15,14,13,12,11,10, \
     9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#ifndef __cplusplus
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#endif // !__cplusplus

#ifdef _WIN32
#define TEBAKO_SET_LAST_ERROR(e)  {                    \
            errno = e;                                 \
			if (e == ENOMEM) {                         \
				SetLastError(ERROR_NOT_ENOUGH_MEMORY); \
                _doserrno = ERROR_NOT_ENOUGH_MEMORY;   \
			} else if (e == ENOENT) {                  \
				SetLastError(ERROR_FILE_NOT_FOUND);    \
                _doserrno = ERROR_FILE_NOT_FOUND;      \
			} else if (e = EBADF) {                    \
				SetLastError(ERROR_INVALID_HANDLE);    \
                _doserrno = ERROR_INVALID_HANDLE;      \
			}                                          \
			else if (e = ENAMETOOLONG) {               \
				SetLastError(ERROR_BUFFER_OVERFLOW);   \
				_doserrno = ERROR_BUFFER_OVERFLOW;     \
			}                                          \
			else if (e == EINVAL) {                    \
				SetLastError(ERROR_BAD_LENGTH);        \
				_doserrno = ERROR_BAD_LENGTH;          \
			}                                          \
			else if (e == ERANGE) {                    \
				SetLastError(ERROR_BUFFER_OVERFLOW);   \
				_doserror = ERROR_BUFFER_OVERFLOW;     \
			} else {                                   \
				SetLastError(ERROR_INVALID_FUNCTION);  \
                _doserrno = ERROR_INVALID_FUNCTION;    \
			}                                          \
		}
#else
#define TEBAKO_SET_LAST_ERROR(e)  errno = e    
#endif

#define TEBAKO_PATH_LENGTH ((size_t) PATH_MAX)
#define TEBAKO_MOINT_POINT "__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_LENGTH  16
 
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef char tebako_path_t[TEBAKO_PATH_LENGTH + 1];

void tebako_set_cwd(const char* path);
const char* tebako_get_cwd(tebako_path_t cwd);
int is_tebako_path(const char* path);
int is_tebako_cwd(void);
const char* tebako_expand_path(tebako_path_t expanded_path, const char* path);

#ifdef __cplusplus
}
#endif // __cplusplus
