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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <tebako-defines.h>
#include <tebako-io.h>
#include "tebako-fs.h"
#include "tests-defines.h"


int main(int argc, char** argv)
{
	struct stat buf;
	char p[PATH_MAX];
	char* r;

	int ret = load_fs(&gfsData[0],
		gfsSize,
		"debug" /*debuglevel*/,
		NULL	/* cachesize*/,
		NULL	/* workers */,
		NULL	/* mlock */,
		NULL	/* decompress_ratio*/,
		NULL    /* image_offset */
	);

	printf("Load file system. ret=%i\n", ret);

	if (ret == 0) {
		ret = stat(TEBAKIZE_PATH("file.txt"), &buf);

		printf("stat. ret=%i, errno=%i\n", ret, errno);

		/* Just test define, comiple and link 
		*/
		access(TEBAKIZE_PATH("file.txt"), F_OK);
		printf("access. ret=%i\n", ret);
		ret=chdir(TEBAKIZE_PATH("directory-1"));
		printf("chdir. ret=%i\n", ret);
		r = getcwd(NULL, 0); free(r);
		printf("getcwd\n");
		r = getwd(p);
		printf("getwd\n");
		mkdir(TEBAKIZE_PATH("directory-1"), S_IRWXU);
		printf("mkdir\n");

		drop_fs();
		printf("drop_fs\n");
	}

	printf("Exiting. ret=%i\n", ret);
	return ret;
}