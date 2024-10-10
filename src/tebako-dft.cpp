/**
 *
 * Copyright (c) 2024, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of tebako
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

#include <tebako-pch.h>
#include <tebako-pch-pp.h>
#include <tebako-common.h>
#include <tebako-dirent.h>
#include <tebako-dfs.h>
#include <tebako-dft.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-mfs.h>
#include <tebako-mnt.h>

using namespace dwarfs;

namespace tebako {

sync_tebako_memfs_table& sync_tebako_memfs_table::get_tebako_memfs_table(void)
{
  static sync_tebako_memfs_table memfs_table{};
  return memfs_table;
}

bool sync_tebako_memfs_table::check(uint32_t index)
{
  auto p_memfs_table = s_tebako_memfs_table.rlock();
  auto p_memfs = p_memfs_table->find(index);
  return (p_memfs != p_memfs_table->end());
}

void sync_tebako_memfs_table::clear(void)
{
  auto p_memfs_table = s_tebako_memfs_table.wlock();
  for (auto& pair : *p_memfs_table) {
    delete pair.second;
  }
  p_memfs_table->clear();
}

void sync_tebako_memfs_table::erase(uint32_t index)
{
  auto p_memfs_table = s_tebako_memfs_table.wlock();
  auto p_memfs = p_memfs_table->extract(index);
  if (p_memfs) {
    delete p_memfs.mapped();
  }
}

memfs* sync_tebako_memfs_table::get(uint32_t index)
{
  auto p_memfs_table = s_tebako_memfs_table.rlock();
  auto p_memfs = p_memfs_table->find(index);
  if (p_memfs != p_memfs_table->end()) {
    return p_memfs->second;
  }
  return nullptr;
}

bool sync_tebako_memfs_table::insert(uint32_t index, memfs* fs)
{
  auto p_memfs_table = s_tebako_memfs_table.wlock();
  return p_memfs_table->emplace(index, fs).second;
}

}  // namespace tebako
