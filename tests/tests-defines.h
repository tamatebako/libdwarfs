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
 *    documentation and/or other matrials provided with the distribution.
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

#pragma once

/*
* defines below are copied from tebako_common.h
* They are copied because !!! the client code should not include tebaco_common.h !!!
*/
#ifdef _WIN32
#define TEBAKO_MOUNT_POINT "\\\\.\\__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_W L"\\\\.\\__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_LENGTH  20
#define TEBAKIZE_PATH(P) TEBAKO_MOUNT_POINT "\\" P
#else
#define TEBAKO_MOUNT_POINT "/__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_W L"/__tebako_memfs__"
#define TEBAKO_MOUNT_POINT_LENGTH  17
#define TEBAKIZE_PATH(P) TEBAKO_MOUNT_POINT "/" P
#endif

#ifdef RB_W32
#define open	        rb_w32_uopen
#define lseek(_f, _o, _w)	rb_w32_lseek(_f, _o, _w)
#define close(h)		rb_w32_close(h)
#define read(f, b, s)	rb_w32_read(f, b, s)

#define chdir(p)        rb_w32_uchdir((p))
#define mkdir(p, m)     rb_w32_umkdir((p), (m))
#define access(p, m)	rb_w32_uaccess((p), (m))
#define fstat(fd,st)	rb_w32_fstati128(fd,st)
#define stat(path, st)	rb_w32_stati128(path,st)
#define lstat(path,st)	rb_w32_lstati128(path,st)
#define getcwd(p, s)    rb_w32_ugetcwd((p), (s))
#define opendir(s)      rb_w32_opendir((s))
#define readdir(d)      rb_w32_readdir((d), 0)
#define telldir(d)      rb_w32_telldir((d))
#define seekdir(d, l)   rb_w32_seekdir((d), (l))
#define rewinddir(d)    rb_w32_rewinddir((d))
#define closedir(d)     rb_w32_closedir((d))
#endif