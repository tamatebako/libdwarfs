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
#include <limits.h>
#include <unistd.h>

#include <tebako-common.h>
#include <tebako-io.h>


#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus
	static tebako_dir_t _tebako_cwd;

/*	
     https://nixdoc.net/man-pages/Linux/man3/getcwd.3.html

     The  getcwd() function copies an absolute pathname of the current working
	 directory to the array pointed to by buf, which is of length size.

	 If  the current  absolute path name would require a buffer longer than
	 size elements, NULL is returned, and errno is set to ERANGE; an application
	 should  check  for  this error, and allocate a larger buffer if
	 necessary.

	 If buf is NULL, the behaviour of getcwd() is undefined.

	 As an extension to the POSIX.1 standard, Linux(libc4, libc5, glibc)
	 getcwd() allocates the buffer dynamically using malloc() if buf is NULL
	 on call. In this case, the allocated buffer has the length size unless
	 size  is zero, when buf is allocated as big as necessary.It is possible
	 (and, indeed, advisable) to free() the buffers if  they  have  been
	 obtained this way.
*/

	char* tebako_getcwd(char* buf, size_t size)
	{
		if (_tebako_cwd[0])
		{
			size_t len = strlen(_tebako_cwd);
			if (!buf)
			{
				buf = strdup(_tebako_cwd);
				if (!buf) 
				{
					errno = ENOMEM;
					TEBAKO_SET_LAST_ERROR(errno);
				}
			}
			else
			{
				if (!size)
				{
					errno = EINVAL;
					buf = NULL;
				}
				else if (len > size - 1)
				{
					errno = ERANGE;
					buf = NULL;
				}
				else
				    strcpy(buf, _tebako_cwd);
			}
			return buf;
		}
		else
   		  return getcwd(buf, size);
	}
/*
	https://nixdoc.net/man-pages/Linux/man3/getcwd.3.html

	getwd, which	  is	only	prototyped    if    _BSD_SOURCE or
	_XOPEN_SOURCE_EXTENDED  is  defined, will not malloc(3) any memory.The
	buf argument should be a pointer to an array at	least  PATH_MAX  bytes
	long. getwd  does  only return the first PATH_MAX bytes of the actual
	pathname.
*/
	char* tebako_getwd(char* buf)
	{
		if (_tebako_cwd[0])
		{
			if (!buf)
			{
				errno = EINVAL;
				buf = NULL;
			}
			else
				strncpy(buf, _tebako_cwd, PATH_MAX);
		}
		else
			return getwd(buf);
		return(buf);
	}
/*
     Internal tebako helper function
	 Sets current working directory to path and removes all extra trailing slashes
*/ 
	void tebako_helper_set_cwd(const char* path)
	{
		if (!path)
		{
			_tebako_cwd[0] = '\0';
		}
		else
		{
			size_t len = min(strlen(path), TEBAKO_PATH_LENGTH - 1);
			memcpy(_tebako_cwd, path, len);
			while (_tebako_cwd[len - 1] == '/') { len--; }
			_tebako_cwd[len] = '/';
			_tebako_cwd[len + 1] = '\0';
		}
	}
#ifdef __cplusplus
}
#endif // !__cplusplus
