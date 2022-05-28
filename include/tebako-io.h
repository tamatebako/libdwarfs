/**
 *
 * Copyright (c) 2021-2022 [Ribose Inc](https://www.ribose.com).
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
#ifdef WITH_GETWD
    char* tebako_getwd(char* buf);
#endif
    int   tebako_chdir(const char* path);

/* Another option -- to be cleaned if 'defined(_SYS_STAT_H)' works
#if defined(__mode_t_defined) || defined(_MODE_T) || defined(__NEED_mode_t)
    __mode_t_defined    -- Ubuntu/GNU
    _MODE_T             -- Darwin
    __NEED_mode_t       -- Alpine/musl
    int   tebako_mkdir(const char* path, mode_t mode);
#endif
*/

    int   tebako_access(const char* path, int amode);
    int   tebako_open(int nargs, const char* path, int flags, ...);
    int   tebako_openat(int nargs, int fd, const char* path, int flags, ...);
    ssize_t tebako_read(int vfd, void* buf, size_t nbyte);

/* struct iovec is defined only if sys/uio.h has been included */
#if defined(_SYS_UIO_H) || defined(_SYS_UIO_H_)
    ssize_t tebako_readv(int vfd, const struct iovec* iov, int iovcnt);
#endif

/* Another option -- to be cleaned if 'defined(_UNISTD_H)' works
#if defined(__off_t_defined) || defined(_OFF_T) || defined(__NEED_off_t)
    __off_t_defined    -- Ubuntu/GNU
    _OFF_T             -- Darwin
    __NEED_off_t       -- Alpine/musl
    ssize_t tebako_pread(int vfd, void* buf, size_t nbyte, off_t offset);
    off_t tebako_lseek(int vfd, off_t offset, int whence);
#endif
*/

#if defined(_UNISTD_H) || defined(_UNISTD_H_)
    ssize_t tebako_pread(int vfd, void* buf, size_t nbyte, off_t offset);
    off_t tebako_lseek(int vfd, off_t offset, int whence);
#endif

/* struct stat is defined only if sys/stat.h has been included */
#if defined(_SYS_STAT_H) || defined(_SYS_STAT_H_)
    int   tebako_mkdir(const char* path, mode_t mode);
    int   tebako_stat(const char* path, struct stat* buf);
    int   tebako_fstat(int vfd, struct stat* buf);
    int   tebako_lstat(const char* path, struct stat* buf);
    int   tebako_fstatat(int fd, const char* path, struct stat* buf, int flag);
#endif

    int   tebako_close(int vfd);
    ssize_t tebako_readlink(const char* path, char* buf, size_t bufsiz);

/* DIR and struct dirent is defined only if dirent.h has been included */
#if defined(_DIRENT_H) || defined(_DIRENT_H_)
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
#endif

    void* tebako_dlopen(const char* path, int flags);
    char* tebako_dlerror(void);

/* struct attr is defined only if sys/attr.h has been included */
#if defined(_SYS_ATTR_H_) || defined(_SYS_ATTR_H_)
    int tebako_getattrlist(const char* path, struct attrlist * attrList, void * attrBuf,  size_t attrBufSize, unsigned long options);
    int tebako_fgetattrlist(int fd, struct attrlist * attrList, void * attrBuf, size_t attrBufSize, unsigned long options);
#endif

    int within_tebako_memfs(const char* path);
#ifdef __cplusplus
}
#endif // __cplusplus
