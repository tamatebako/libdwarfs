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
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <tebako-common.h>
#include <tebako-io.h>


#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus

/*
*	https://pubs.opengroup.org/onlinepubs/9699919799/
* 
*	DESCRIPTION
*	The chdir() function shall cause the directory named by the pathname pointed to by the path argument to become the current working directory; that is, the starting point for path searches for pathnames not beginning with '/'.
*
*	RETURN VALUE
*	Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned, the current working directory shall remain unchanged, and errno shall be set to indicate the error.
*
*	ERRORS
*	The chdir() function shall fail if:
*
*		[EACCES] Search permission is denied for any component of the pathname.
*		[ELOOP]  A loop exists in symbolic links encountered during resolution of the path argument.
*		[ENAMETOOLONG] The length of a component of a pathname is longer than {NAME_MAX}.
*		[ENOENT] A component of path does not name an existing directory or path is an empty string.
*		[ENOTDIR] A component of the pathname names an existing file that is neither a directory nor a symbolic link to a directory.
*
*	The chdir() function may fail if:
*
*		[ELOOP] More than {SYMLOOP_MAX} symbolic links were encountered during resolution of the path argument.
*		[ENAMETOOLONG] The length of a pathname exceeds {PATH_MAX}, or pathname resolution of a symbolic link produced an intermediate result with a length that exceeds {PATH_MAX}.
*/
	int tebako_chdir(const char* path)
	{
		int ret = -1;
		if (is_tebako_path(path)) 
		{			
			struct stat st;
			
//			ret = tebako_stat(ruby_packer_fs, path, &st);
			if (ret == -1) 
			{
				TEBAKO_SET_LAST_ERROR(errno);	
			}
			else if (S_ISDIR(st.st_mode)) 
			{
				tebako_set_cwd(path);
				ret = 0;	
			}
			else 
			{
				TEBAKO_SET_LAST_ERROR(errno);
			}			
		}
		else 
		{
			ret = chdir(path);
			if (ret == 0) tebako_set_cwd(NULL);
		}
		return ret;
	}
#ifdef __cplusplus
}
#endif // !__cplusplus
