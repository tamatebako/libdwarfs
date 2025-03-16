/**
 *
 * Copyright (c) 2021-2025, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of the Tebako project. (libdwarfs-wr)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other matrials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

/*
 * defines below are copied from tebako_common.h
 * They are copied because !!! the client code should not include
 * tebaco_common.h !!!
 */

#ifdef _WIN32
#define TEBAKO_MOUNT_POINT "A:\\__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_W L"A:\\__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_LENGTH 19
#define TEBAKIZE_PATH(P) TEBAKO_MOUNT_POINT "\\" P
#else
#define TEBAKO_MOUNT_POINT "/__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_W L"/__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_LENGTH 17
#define TEBAKIZE_PATH(P) TEBAKO_MOUNT_POINT "/" P
#endif

#ifdef _WIN32

#ifndef F_OK
#define F_OK 0
#endif

#ifndef W_OK
#define W_OK 2
#endif

#ifndef R_OK
#define R_OK 4
#endif

#ifndef X_OK
#define X_OK F_OK
#endif

#ifndef S_IRUSR
#define S_IRUSR S_IREAD /* read, user */
#endif

#ifndef S_IWUSR
#define S_IWUSR S_IWRITE /* write, user */
#endif

#ifndef S_IXUSR
#define S_IXUSR 0 /* execute, user */
#endif

#ifndef S_IRGRP
#define S_IRGRP 0 /* read, group */
#endif

#ifndef S_IWGRP
#define S_IWGRP 0 /* write, group */
#endif

#ifndef S_IXGRP
#define S_IXGRP 0 /* execute, group */
#endif

#ifndef S_IROTH
#define S_IROTH 0 /* read, others */
#endif

#ifndef S_IWOTH
#define S_IWOTH 0 /* write, others */
#endif

#ifndef S_IXOTH
#define S_IXOTH 0 /* execute, others */
#endif

#ifndef S_IRWXU
#define S_IRWXU 0
#endif

#ifndef S_IRWXG
#define S_IRWXG 0
#endif

#ifndef S_IRWXO
#define S_IRWXO 0
#endif

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

#define open rb_w32_uopen
#define lseek(_f, _o, _w) rb_w32_lseek(_f, _o, _w)
#define close(h) rb_w32_close(h)
#define read(f, b, s) rb_w32_read(f, b, s)
#define pread(f, b, s, o) rb_w32_pread(f, b, s, o)

#define chdir(p) rb_w32_uchdir((p))
#define mkdir(p, m) rb_w32_umkdir((p), (m))
#define rmdir(p) rb_w32_urmdir((p))
#define unlink(p) rb_w32_uunlink((p))
#define access(p, m) rb_w32_uaccess((p), (m))
#define fstat(fd, st) rb_w32_fstati128(fd, st)
#define stat(path, st) rb_w32_stati128(path, st)
#define lstat(path, st) rb_w32_lstati128(path, st)
#define getcwd(p, s) rb_w32_ugetcwd((p), (s))
#define opendir(s) rb_w32_opendir((s))
#define readdir(d) rb_w32_readdir((d), 0)
#define telldir(d) rb_w32_telldir((d))
#define seekdir(d, l) rb_w32_seekdir((d), (l))
#define rewinddir(d) rb_w32_rewinddir((d))
#define closedir(d) rb_w32_closedir((d))
#endif