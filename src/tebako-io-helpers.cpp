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
#include <folly/Synchronized.h>


//	Current working direcory (within tebako memfs)
//  RW lock implemented with folly tooling
//  github.com/facebook/folly/blob/master/folly/docs/Synchronized
 
struct tebako_path_s {
	tebako_path_t d;
	tebako_path_s(void) { d[0] = '\0'; }
};
static folly::Synchronized<tebako_path_s*> tebako_cwd{ new tebako_path_s };

//	Gets current working directory 
	extern "C" const char* tebako_get_cwd(tebako_path_t cwd) {
		auto locked = tebako_cwd.rlock();
		auto p = *locked;
		return strcpy(cwd, p->d);
	}

//	Sets current working directory to path and removes all extra trailing slashes
//  [TODO: Canonical ?]
	extern "C" void tebako_set_cwd(const char* path) {
		auto locked = tebako_cwd.wlock();
		auto p = *locked;
		if (!path) {
			p->d[0] = '\0';
		}
		else {
			size_t len = std::min(strlen(path), TEBAKO_PATH_LENGTH);
			memcpy(p->d, path, len);
			while (p->d[len - 1] == '/') { len--; }
			p->d[len] = '/';
			p->d[len + 1] = '\0';
		}
	}

//	Checks if a path is withing tebako memfs
//  [TODO: Canonical ?]
	extern "C" int is_tebako_path(const char* path) {
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

//	Checks if the current cwd path is withing tebako memfs
	extern "C" int is_tebako_cwd(void) {
		auto locked = tebako_cwd.rlock();
		auto p = *locked;
		return 	(p->d[0] == '\0') ? 0 : -1;
	}

//	 Expands a path withing tebako memfs
//  [TODO: Canonical ?]
	extern "C" const char* tebako_expand_path(tebako_path_t expanded_path, const char* path) {
		size_t cwd_len;
		{
			auto locked = tebako_cwd.rlock();
			auto p = *locked;
			cwd_len = strlen(p->d);
			memcpy(expanded_path, p->d, cwd_len);
		}

		size_t path_len = std::min(strlen(path), TEBAKO_PATH_LENGTH - cwd_len);
		memcpy(expanded_path + cwd_len, path, path_len);
		expanded_path[cwd_len + path_len] = '\0';
		return expanded_path;
	}


