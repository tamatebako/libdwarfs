/**
 *
 * Copyright (c) 2022-2024, [Ribose Inc](https://www.ribose.com).
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

namespace tebako {

// sync_tebako_kfdtable
// This class manages a set of DIR* pointer created by Tebako when processing
// opendir calls. Directory functions will fail if an alien DIR pointer is
// passed to them, so we must filter out Tebako handles using this set. DIR* is
// converted to uintptr_t to simplify portability since DiR is defined
// differently in POSIX vs Windows vs Windows MinGW vs Ruby core on Windows and
// we do not ptoagate tons of #ifdefs everywhere Refer to dir_io.cpp for usage
// details.

typedef std::set<uintptr_t> tebako_kfdtable;

class sync_tebako_kfdtable {
 private:
  folly::Synchronized<tebako_kfdtable> s_tebako_kfdtable;

 public:
  static sync_tebako_kfdtable& get_tebako_kfdtable(void);

  bool check(uintptr_t dirp);
  void clear(void);
  void erase(uintptr_t dirp);
  void insert(uintptr_t dirp);
};

}  // namespace tebako
