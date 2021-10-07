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

#include <tebako-defines.h>
#include <stdlib.h>
#include <stdio.h>
#include <tebako-io.h>
#include "tebako-fs.h"
#include "tests-defines.h"


int main(int argc, char** argv)
{
	struct stat buf;
	char p[PATH_MAX];
	char* r;
	const int true = -1, false = 0;
	int rOK = false; 

	int ret = load_fs(&gfsData[0],
		gfsSize,
		"debug" /*debuglevel*/,
		NULL	/* cachesize*/,
		NULL	/* workers */,
		NULL	/* mlock */,
		NULL	/* decompress_ratio*/,
		NULL    /* image_offset */
	);

	printf("A call to load_fs returned %i\n", ret);

	if (ret == 0) {
		rOK = true;

		/* 
		* Positive cases only, just to check tha define and link works correctly
		* The real unit tests are done by gtest (wr-tests) 
		*/

		ret = stat(TEBAKIZE_PATH("file.txt"), &buf);
		printf("A call to 'stat' returned %i\n", ret);
		rOK &= (ret == 0);

		ret = access(TEBAKIZE_PATH("file.txt"), F_OK);
		printf("A call to 'access' returned %i\n", ret);
		rOK &= (ret == 0);

		ret = chdir(TEBAKIZE_PATH("directory-1"));
		printf("A call to 'chdir' returned %i\n", ret);
		rOK &= (ret == 0);

		r = getcwd(NULL, 0); 
		printf("A call to 'getcwd' returned %p\n", r);
		rOK &= (r != NULL);
		free(r);

		r = getwd(p);
		printf("A call to 'getwd' returned %p\n", r);
		rOK &= (r != NULL);

		ret = mkdir(TEBAKIZE_PATH("directory-1"), S_IRWXU);
		printf("A call to 'mkdir' returned %i\n", ret);
		rOK &= (ret == -1);

		drop_fs();
		printf("Filesystem dropped\n");
		
		ret = rOK ? 0 : -1;
	}

	printf("Exiting. Return code=%i\n", ret);
	return ret;
}