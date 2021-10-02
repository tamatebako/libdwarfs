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
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <tebako-common.h>


#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus
/*
	Current working direcory (within tebako memfs)
*/
	static tebako_path_t tebako_cwd;

/*
	 Gets current working directory 
*/
	const char* tebako_get_cwd(void)
	{
		return tebako_cwd;
	}

/*
	 Sets current working directory to path and removes all extra trailing slashes
*/ 
	void tebako_set_cwd(const char* path)
	{
		if (!path)
		{
			tebako_cwd[0] = '\0';
		}
		else
		{
			size_t len = min(strlen(path), TEBAKO_PATH_LENGTH - 1);
			memcpy(tebako_cwd, path, len);
			while (tebako_cwd[len - 1] == '/') { len--; }
			tebako_cwd[len] = '/';
			tebako_cwd[len + 1] = '\0';
		}
	}

	int is_tebako_path(const char* path)
	{
		return (strncmp((path), "/" TEBAKO_MOINT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 1) == 0
#ifdef _WIN32
			|| strncmp(path, "\\" TEBAKO_MOINT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 1) == 0
			|| strncmp(path + 1, ":/" TEBAKO_MOINT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 2) == 0
			|| strncmp(path + 1, ":\\" TEBAKO_MOINT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 2) == 0
			|| strncmp(path, "//?/" TEBAKO_MOINT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 3) == 0
			|| strncmp(path, "\\\\?\\" TEBAKO_MOINT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 3) == 0
			|| ((strncmp(path, "\\\\?\\", 4) == 0 || strncmp(path, "//?/", 4) == 0) &&
				(strncmp(path + 5, ":/" TEBAKO_MOINT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 2) == 0 ||
					strncmp(path + 5, ":\\" TEBAKO_MOINT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 2) == 0
					)
#endif
			) ? -1 : 0;
	}

#ifdef __cplusplus
}
#endif // !__cplusplus
