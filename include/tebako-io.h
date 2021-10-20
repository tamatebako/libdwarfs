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

 /*
 *  Tebako filesystem interface functions
 *  This file shall be included into sources we want to "hack"
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus
    int load_fs(const void* data,
        const unsigned int size,
        const char* debuglevel,
        const char* cachesize,
        const char* workers,
        const char* mlock,
        const char* decompress_ratio,
        const char* image_offset
    );

    void drop_fs(void);

    char* tebako_getcwd(char* buf, size_t size);
    char* tebako_getwd(char* buf);
    int   tebako_chdir(const char* path);
    int   tebako_mkdir(const char* path, mode_t mode);
    int   tebako_stat(const char* path, struct stat* buf);
    int   tebako_access(const char* path, int amode);
    int   tebako_open(int nargs, const char* path, int flags, ...);
    int   tebako_openat(int nargs, int fd, const char* path, int flags, ...);
    ssize_t tebako_read(int vfd, void* buf, size_t nbyte);
    ssize_t tebako_readv(int vfd, const struct iovec* iov, int iovcnt);
    ssize_t tebako_pread(int vfd, void* buf, size_t nbyte, off_t offset);
    off_t tebako_lseek(int vfd, off_t offset, int whence);
    int   tebako_fstat(int vfd, struct stat* buf);
    int   tebako_close(int vfd);

    DIR* tebako_opendir(const char* dirname);
    DIR* tebako_fdopendir(int fd);
    struct dirent* tebako_readdir(DIR* dirp);
    long tebako_telldir(DIR* dirp);
    void tebako_seekdir(DIR* dirp, long loc);
    void tebako_rewinddir(DIR* dirp);
    int tebako_closedir(DIR* dirp);
    int tebako_dirfd(DIR* dirp);
    int tebako_scandir(const char* dir, struct dirent*** namelist,
        int (*sel)(const struct dirent*),
        int (*compar)(const struct dirent**, const struct dirent**));

#ifdef __cplusplus
}
#endif // __cplusplus
