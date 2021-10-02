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
#include <tebako-io.h>


#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus
/*	
*   https://pubs.opengroup.org/onlinepubs/9699919799/
* 
*	DESCRIPTION
*	The getcwd() function shall place an absolute pathname of the current working directory in the array pointed to by buf, and return buf. The pathname copied to the array shall contain no components that are symbolic links.
*   The size argument is the size in bytes of the character array pointed to by the buf argument. If buf is a null pointer, the behavior of getcwd() is unspecified.
*
*	RETURN VALUE
*	Upon successful completion, getcwd() shall return the buf argument. Otherwise, getcwd() shall return a null pointer and set errno to indicate the error. The contents of the array pointed to by buf are then undefined.
*
*	ERRORS
*	The getcwd() function shall fail if:
*
*		[EINVAL] The size argument is 0.
*		[ERANGE] The size argument is greater than 0, but is smaller than the length of the pathname +1.
* 
*	The getcwd() function may fail if:
*
*		[EACCES] Read or search permission was denied for a component of the pathname.
*		[ENOMEM] Insufficient storage space is available.

*	 As an extension to the POSIX.1 standard, Linux(libc4, libc5, glibc) getcwd() allocates the buffer dynamically using malloc() if buf is NULL
*	 on call. In this case, the allocated buffer has the length size unless size  is zero, when buf is allocated as big as necessary.It is possible
*	 (and, indeed, advisable) to free() the buffers if  they  have  been obtained this way.
*/

	char* tebako_getcwd(char* buf, size_t size)
	{
		const char* cwd = tebako_get_cwd();
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
						buf = malloc(size);
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
   		  return getcwd(buf, size);
	}
/*
*	LEGACY, DEPRECATED
*	https://pubs.opengroup.org/onlinepubs/009695299/functions/getwd.html
* 
*	DESCRIPTION
*	The getwd() function shall determine an absolute pathname of the current working directory of the calling process, and copy a string containing that pathname into the array pointed to by the path_name argument.
*
*	If the length of the pathname of the current working directory is greater than ({PATH_MAX}+1) including the null byte, getwd() shall fail and return a null pointer.
*
*	RETURN VALUE
*	Upon successful completion, a pointer to the string containing the absolute pathname of the current working directory shall be returned. Otherwise, getwd() shall return a null pointer and the contents of the array pointed to by path_name are undefined.
*
*	ERRORS
*	No errors are defined.
*/
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif 
	char* tebako_getwd(char* buf)
	{
		const char* cwd = tebako_get_cwd();
		return (cwd[0])? strcpy(buf, cwd): getwd(buf);
	}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif 

#ifdef __cplusplus
}
#endif // !__cplusplus
