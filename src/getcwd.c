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


#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus
/*	
*   getcwd()
*   https://pubs.opengroup.org/onlinepubs/9699919799/
*/

	char* tebako_getcwd(char* buf, size_t size) {
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
*   getwd()
*	LEGACY, DEPRECATED
*	https://pubs.opengroup.org/onlinepubs/009695299/functions/getwd.html
*/
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif 
	char* tebako_getwd(char* buf) {
		if (is_tebako_cwd()) {
			tebako_get_cwd(buf);
			return buf;
		} 
		else {
			return getwd(buf);
		}
	}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif 

#ifdef __cplusplus
}
#endif // !__cplusplus
