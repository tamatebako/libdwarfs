/**
 *
 * Copyright (c) 2021-2022, [Ribose Inc](https://www.ribose.com).
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

#include <tebako-pch.h>
#include <tebako-defines.h>
#include <stdlib.h>
#include <stdio.h>
#include <tebako-io.h>
#include "tebako-fs.h"
#include "tests-defines.h"

#ifndef true
#define true 1
#define false 0
#endif

static int attr_functions_c_test(char* fname);
static int pread_c_test(int fh);
static int readv_c_test(int fh);
static int lseek_read_c_test(int fh);
static int open_3_args_c_test(void);
static int openat_c_test(int fh);
static int dirio_c_test(void);
static int dirio_fd_c_test(void);
static int scandir_c_test(void);
static int dlopen_c_test(void);
static int link_c_tests(void);
static int fstatat_c_test(void);

int main(int argc, char** argv)
{
	char p[PATH_MAX];
	char* r;
	int rOK = false;
	int fh;
	struct stat buf;

	int ret = load_fs(&gfsData[0],
		gfsSize,
		tests_log_level,
		NULL	/* cachesize*/,
		NULL	/* workers */,
		NULL	/* mlock */,
		NULL	/* decompress_ratio*/,
		NULL    /* image_offset */
	);

	printf("A call to load_fs returned %i (0 expected)\n", ret);

	if (ret == 0) {
		rOK = true;

		rOK &= attr_functions_c_test(TEBAKIZE_PATH("file.txt"));
        if (!rOK) printf("failing\n");

		ret = chdir(TEBAKIZE_PATH("directory-1"));
		printf("A call to 'chdir' returned %i (0 expected)\n", ret);
		rOK &= (ret == 0);
        if (!rOK) printf("failing\n");

		r = getcwd(NULL, 0);
		printf("A call to 'getcwd' returned %p (not NULL expected)\n", r);
		rOK &= (r != NULL);
		free(r);
        if (!rOK) printf("failing\n");

#ifdef WITH_GETWD
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
		r = getwd(p);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
		printf("A call to 'getwd' returned %p (not NULL expected)\n", r);
		rOK &= (r != NULL);
        if (!rOK) printf("failing\n");
#endif
#ifndef _WIN32
		ret = mkdir(TEBAKIZE_PATH("directory-1"), S_IRWXU);
#else
		ret = mkdir(TEBAKIZE_PATH("directory-1"));
#endif
		printf("A call to 'mkdir' returned %i (-1 expected)\n", ret);
		rOK &= (ret == -1);
        if (!rOK) printf("failing\n");

		fh = open(TEBAKIZE_PATH("file.txt"), O_RDONLY);
		printf("A call to 'open' returned %i (non negative file handle expected)\n", fh);
		rOK &= (fh >= 0);
        if (!rOK) printf("failing\n");

		rOK &= lseek_read_c_test(fh);
		if (!rOK) printf("failing\n");
		rOK &= readv_c_test(fh);  /* Skipped 'Ju', read 'st', ' a file' remains */
        if (!rOK) printf("failing\n");
		rOK &= pread_c_test(fh);
        if (!rOK) printf("failing\n");
		rOK &= openat_c_test(fh);
        if (!rOK) printf("failing\n");

		ret = fstat(fh, &buf);
		rOK &= (ret == 0);
        if (!rOK) printf("failing\n");

		ret = close(fh);
		printf("A call to 'close' returned %i (0 expected)\n", ret);
		rOK &= (ret == 0);
        if (!rOK) printf("failing\n");

		rOK &= open_3_args_c_test();
        if (!rOK) printf("failing\n");
		rOK &= dirio_c_test();
        if (!rOK) printf("failing\n");
		rOK &= dirio_fd_c_test();
        if (!rOK) printf("failing\n");
		rOK &= scandir_c_test();
        if (!rOK) printf("failing\n");
		rOK &= dlopen_c_test();
        if (!rOK) printf("failing\n");
		rOK &= link_c_tests();
        if (!rOK) printf("failing\n");

		rOK &= fstatat_c_test();
        if (!rOK) printf("failing\n");

		drop_fs();
		printf("Filesystem dropped\n");

		ret = rOK ? 0 : -1;
	}

	printf("Exiting. Return code: %i\n", ret);
	return ret;
}

static int attr_functions_c_test(char* fname) {
	struct stat buf;
	int rOK = true;
	int ret = stat(fname, &buf);
	printf("A call to 'stat' returned %i (0 expected)\n", ret);
	rOK &= (ret == 0);

	ret = access(fname, F_OK);
	printf("A call to 'access' returned %i (0 expected)\n", ret);
	rOK &= (ret == 0);

	return rOK;
}

static int openat_c_test(int fh) {
	int rOK = true;
#ifdef TEBAKO_HAS_OPENAT
	int fh2 = openat(fh, TEBAKIZE_PATH("file2.txt"), O_RDONLY);
	printf("A call to 'openat' returned %i (non negative file handle expected expected)\n", fh2);

	int ret = close(fh2);
	printf("A call to 'close' returned %i (0 expected)\n", ret);
	rOK = ( fh2 > 0 && ret == 0 );
#endif
	return rOK;
}

static int lseek_read_c_test(int fh) {
	int ret;
	char readbuf[32];
	int rOK = true;
	ret = lseek(fh, 2, SEEK_SET);
	printf("A call to 'lseek' returned %i (2 expected)\n", ret);
	rOK &= (ret == 2);

	ret = read(fh, readbuf, 2); readbuf[2] = '\0';
	printf("A call to 'read' returned %i (2 expected); Read buffer: '%s' ('st' expected)\n", ret, readbuf);
	rOK &= (ret == 2);
	rOK &= (strcmp(readbuf, "st") == 0);
	return rOK;
}

static int pread_c_test(int fh) {
	int rOK = true;
#ifdef TEBAKO_HAS_PREAD
	int ret;
	char readbuf[32];
	ret = pread(fh, readbuf, 4, 7); readbuf[4] = '\0';
	printf("A call to 'pread' returned %i (4 expected); Read buffer: '%s' ('file' expected)\n", ret, readbuf);
	rOK &= (ret == 4);
	rOK &= (strcmp(readbuf, "file") == 0);
#endif
	return rOK;
}

static int readv_c_test(int fh) {
	int rOK = true;
#ifdef TEBAKO_HAS_READV
	ssize_t s;
	const int buflen = 5;
	char buf0[buflen];
	char buf1[buflen];
	char buf2[buflen];
	int iovcnt;
	struct iovec iov[3];
	char* pattern = " a file";
	int l = strlen(pattern);

	iov[0].iov_base = buf0;
	iov[0].iov_len = buflen;
	iov[1].iov_base = buf1;
	iov[1].iov_len = buflen;
	iov[2].iov_base = buf2;
	iov[2].iov_len = buflen;
	iovcnt = sizeof(iov) / sizeof(struct iovec);

	s = readv(fh, &iov[0], iovcnt);
	printf("A call to 'readv' returned %li (7 expected)\n", s);      /* Skipped 'Ju', read 'st', ' a file' remains */
	rOK &= (s == 7);

	printf("buf0 = '%.*s'(' a fi' expected); buf1 = '%.*s'('le' expected)\n", buflen, buf0, l-buflen, buf1);
	rOK &= (strncmp(buf0, pattern, buflen)==0);
	rOK &= (strncmp(buf1, pattern + buflen, l-buflen)==0);
#endif
	return rOK;
}

static int open_3_args_c_test(void) {
	int rOK = true;
	int ret = unlink("/tmp/some-tebako-test-file.txt");
	printf("A call to 'unlink' returned %i (-1 supposed but 0 is also possible)\n", ret);
	// ret is not checked intensionally !!!

	ret = open("/tmp/some-tebako-test-file.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	printf("A call to 'open' with 3 arguments returned %i (non negative file handle expected)\n", ret);
	rOK &= (ret >= 0);

	ret = close(ret);
	printf("A call to 'close' returned %i (0 expected)\n", ret);
	rOK &= (ret == 0);

	ret = unlink("/tmp/some-tebako-test-file.txt");
	printf("A call to 'unlink' returned %i (0 expected)\n", ret);
	rOK &= (ret == 0);

	return rOK;
}

static int dirio_c_test(void) {
	int rOK = true;
	DIR* dirp = opendir(TEBAKIZE_PATH("directory-2"));
	printf("A call to 'opendir' returned %p (not NULL expected)\n", dirp);
	rOK &= (dirp != NULL);

	long pos = telldir(dirp);
	printf("A call to 'telldir' returned %li (0 expected)\n", pos);
	rOK &= (pos == 0L);

	seekdir(dirp, 2);
	pos = telldir(dirp);
	printf("A call to 'telldir' after 'seekdir(dirp, 2)' returned %li (2 expected)\n", pos);
	rOK &= (pos == 2L);

	struct dirent* entry = readdir(dirp);
	printf("A call to 'readdir'  returned %p (not NULL expected)\n", entry);
	rOK &= (entry != NULL);
	if (entry != NULL) {
		printf("Filename: %s ('file-in-directory-2.txt' expected)\n", entry->d_name);
		rOK &= (strcmp(entry->d_name, "file-in-directory-2.txt") == 0);
	}

	rewinddir(dirp);
	pos = telldir(dirp);
	printf("A call to 'telldir' after 'rewinddir(dirp)' returned %li (0 expected)\n", pos);
	rOK &= (pos == 0L);

	int ret = closedir(dirp);
	printf("A call to 'closedir' returned %i (0 expected)\n", ret);
	rOK &= (ret == 0);

	return rOK;
}

static int dirio_fd_c_test(void) {
	int rOK = true;
#if defined(TEBAKO_HAS_FDOPENDIR) && defined(TEBAKO_HAS_DIRFD)
	int fh = open(TEBAKIZE_PATH("directory-1"), O_RDONLY|O_DIRECTORY);
	printf("A call to 'open' returned %i (non negative file handle expected)\n", fh);
	rOK &= (fh >= 0);

	DIR* dirp = fdopendir(fh);
	printf("A call to 'opendir' returned %p (not NULL expected)\n", dirp);
	rOK &= (dirp != NULL);

	int fh2 = dirfd(dirp);
	printf("A call to 'dirfd' returned %i (%i expected)\n", fh, fh2);
	rOK &= (fh == fh2);

	int ret = closedir(dirp);
	printf("A call to 'closedir' returned %i (0 expected)\n", ret);
	rOK &= (ret == 0);
#endif
	return rOK;
}

static int scandir_c_test(void) {
	int rOK = true;
#ifdef TEBAKO_HAS_SCANDIR
	struct dirent** namelist;
	int n, i;

	n = scandir(TEBAKIZE_PATH("directory-1"), &namelist, NULL, alphasort);
	printf("A call to 'scandir' returned %i (5 expected)\n", n);
	rOK &= (n == 5);
	if ( n>0 ) {
		for (i = 0; i < n; i++) {
			printf("Scandir file name #%i: '%s'\n", i, namelist[i]->d_name);
			free(namelist[i]);
		}
		free(namelist);
	}
#endif
	return rOK;
}

static int dlopen_c_test(void) {
	int rOK = true;
	void* handle = dlopen(TEBAKIZE_PATH("directory-1/libempty.so"), RTLD_LAZY | RTLD_GLOBAL);
	rOK &= (handle != NULL);
	printf("A call to 'dlopen' returned %p (not NULL expected)\n", handle);
	if (handle != NULL) {
		int ret = dlclose(handle);
		printf("A call to 'dlclose' returned %i (0 expected)\n", ret);
		rOK &= (ret == 0);
	}
	return rOK;
}

static int link_c_tests(void) {
	int rOK = true;
#if defined(WITH_LINK_TESTS) && defined(TEBAKO_HAS_LSTAT)
	struct stat st;
	char buf[256];
	int ret = lstat(TEBAKIZE_PATH("s-link-to-file-1"), &st);
	printf("A call to 'lstat' returned %i (0 expected)\n", ret);
	rOK &= (ret == 0);

	ret = readlink(TEBAKIZE_PATH("s-link-to-file-1"), buf, sizeof(buf) / sizeof(buf[0]));
	printf("A call to 'readlink' returned %i (35 expected)\n", ret);
	rOK &= (ret == 35);
#endif
	return rOK;
}

static int fstatat_c_test(void) {
	int rOK = true;
#ifdef TEBAKO_HAS_FSTATAT
	struct stat buf;
	int ret = chdir(TEBAKIZE_PATH("directory-2"));
	rOK &= (ret == 0);
	ret = fstatat(AT_FDCWD, "file-in-directory-2.txt", &buf, 0);
	rOK &= (ret == 0);
#endif
	return rOK;
}
