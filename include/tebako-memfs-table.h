/**
 *
 * Copyright (c) 2024 [Ribose Inc](https://www.ribose.com).
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

typedef std::map<uint32_t, std::shared_ptr<memfs>> tebako_memfs_table;

class sync_tebako_memfs_table {
 private:
  folly::Synchronized<tebako_memfs_table> s_tebako_memfs_table;

 public:
  static sync_tebako_memfs_table& get_tebako_memfs_table(void);

  bool check(uint32_t index);
  void clear(void);
  void erase(uint32_t index);
  std::shared_ptr<memfs> get(uint32_t index);
  bool insert(uint32_t index, std::shared_ptr<memfs> fs);
  uint32_t insert_auto(std::shared_ptr<memfs> fs);
};

template <typename Functor, class... Args>
int memfs_call(Functor&& fn, uint32_t fs_index, Args&&... args)
{
  int ret = DWARFS_IO_ERROR;

  auto fs = sync_tebako_memfs_table::get_tebako_memfs_table().get(fs_index);
  if (fs == nullptr) {
    TEBAKO_SET_LAST_ERROR(ENOENT);
  }
  else {
    ret = ((*fs).*fn)(std::forward<Args>(args)...);
  }
  return ret;
}

template <typename Functor, class... Args>
int root_memfs_call(Functor&& fn, Args&&... args)
{
  return memfs_call(fn, 0, std::forward<Args>(args)...);
}

template <typename Functor, class... Args>
int inode_memfs_call(Functor&& fn, uint32_t inode, Args&&... args)
{
  int ret = DWARFS_IO_ERROR;
  uint32_t fs_index = (inode >> 29) & 0x7;  // Higher three bits is memfs index

  auto fs = sync_tebako_memfs_table::get_tebako_memfs_table().get(fs_index);
  if (fs == nullptr) {
    TEBAKO_SET_LAST_ERROR(ENOENT);
  }
  else {
    ret = ((*fs).*fn)(inode, std::forward<Args>(args)...);
  }
  return ret;
}

}  // namespace tebako
