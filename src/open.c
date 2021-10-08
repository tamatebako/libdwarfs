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
#include <tebako-io.h>
#include <tebako-io-inner.h>

 /*
 * int tebako_open(int nargs, const char* pathname, int flags, ...)
 * https://pubs.opengroup.org/onlinepubs/9699919799/
 *
 */

int tebako_open(int nargs, const char* path, int flags, ...)
{
	int ret = -1;
	const char* p_path = NULL;
	tebako_path_t t_path;
	if (is_tebako_cwd() && path[0] != '/') {
		p_path = tebako_expand_path(t_path, path);
	}
	else if (is_tebako_path(path)) {
		p_path = path;
	}

	if (p_path) {
		if (flags & O_RDWR || flags & O_WRONLY) {
			TEBAKO_SET_LAST_ERROR(EROFS);
		}
		else {
			ret = dwarfs_find(p_path);
		}
	}
	else {
		if (nargs == 2) {
			ret = open(path, flags);
		}
		else {
			va_list args;
			mode_t mode;
			va_start(args, flags);
			mode = va_arg(args, mode_t);
			va_end(args);
			ret = open(path, flags, mode);
		}
	}
	return ret;
}