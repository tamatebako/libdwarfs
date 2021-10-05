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

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <tebako-common.h>
#include <tebako-io.h>


#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus

/*
* chdir()
* https://pubs.opengroup.org/onlinepubs/9699919799/
* 
*/
	int tebako_chdir(const char* path)	{
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
			struct stat st;
			ret = tebako_stat(p_path, &st);
			if (ret == 0) 	{
				if (S_ISDIR(st.st_mode)) {
					tebako_set_cwd(p_path);
				}
				else {
					ret = -1;
					TEBAKO_SET_LAST_ERROR(ENOTDIR);
				}			
			}
		}
		else {
			ret = chdir(path);
			if (ret == 0) {
				tebako_set_cwd(NULL);
			}
		}
		return ret;
	}
#ifdef __cplusplus
}
#endif // !__cplusplus
