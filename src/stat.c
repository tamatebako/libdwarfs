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

+#define RUBY_PACKER_GEN_EXPANDED_NAME(path)	\
+			ruby_packer_cwd_len = strlen(ruby_packer_cwd); \
+			memcpy(ruby_packer_expanded, ruby_packer_cwd, ruby_packer_cwd_len); \
+			memcpy_len = strlen(path); \
+			if (SQUASHFS_PATH_LEN - ruby_packer_cwd_len < memcpy_len) { memcpy_len = SQUASHFS_PATH_LEN - ruby_packer_cwd_len; } \
+			memcpy(&ruby_packer_expanded[ruby_packer_cwd_len], (path), memcpy_len); \
+			ruby_packer_expanded[ruby_packer_cwd_len + memcpy_len] = '\0'


int tebako_stat(const char* path, struct stat* buf)
{
	if (ruby_packer_cwd[0] && '/' != *path) {
		sqfs_path ruby_packer_expanded;
		size_t ruby_packer_cwd_len;
		size_t memcpy_len;
		RUBY_PACKER_GEN_EXPANDED_NAME(path);
		RUBY_PACKER_CONSIDER_MKDIR_WORKDIR_RETURN(
			ruby_packer_expanded,
			ruby_packer_dos_return(squash_stat(ruby_packer_fs, ruby_packer_expanded, buf)),
			stat(mkdir_workdir_expanded, buf)
		);
	}
	else if (ruby_packer_is_path(path)) {
		RUBY_PACKER_CONSIDER_MKDIR_WORKDIR_RETURN(
			path,
			ruby_packer_dos_return(squash_stat(ruby_packer_fs, path, buf)),
			stat(mkdir_workdir_expanded, buf)
			);
		
	}
	else {
		return stat(path, buf);
		
	}
}
