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

#include <tebako-mnt.h>

namespace tebako {
sync_tebako_mount_table& sync_tebako_mount_table::get_tebako_mount_table(void)
{
  static sync_tebako_mount_table mount_table{};
  return mount_table;
}

bool sync_tebako_mount_table::check(std::string& path)
{
  auto p_mount_table = s_tebako_mount_table.rlock();
  auto p_mount = p_mount_table->find(path);
  return (p_mount != p_mount_table->end());
};

void sync_tebako_mount_table::clear(void)
{
  auto p_mount_table = s_tebako_mount_table.wlock();
  p_mount_table->clear();
}

void sync_tebako_mount_table::erase(std::string& path)
{
  auto p_mount_table = s_tebako_mount_table.wlock();
  p_mount_table->erase(path);
}

std::optional<std::string> sync_tebako_mount_table::get(std::string& path)
{
  auto p_mount_table = s_tebako_mount_table.rlock();
  auto p_mount = p_mount_table->find(path);
  std::optional<std::string> ret = std::nullopt;
  if (p_mount != p_mount_table->end()) {
    ret = p_mount->second;
  }
  return ret;
}

bool sync_tebako_mount_table::insert(std::string& path, std::string& mount)
{
  auto p_mount_table = s_tebako_mount_table.wlock();
  const auto [it, success] = p_mount_table->insert(std::make_pair(path, mount));
  return success;
}

}  // namespace tebako
