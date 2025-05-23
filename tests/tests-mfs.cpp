/**
 *
 * Copyright (c) 2021-2025, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of the Tebako project.
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

#include <cerrno>
#include <string>

#include <folly/portability/SysMman.h>
#include <folly/portability/Unistd.h>
#include <boost/system/error_code.hpp>

#include <tebako-pch.h>
#include <tebako-common.h>

#include <tebako-mfs.h>

namespace tebako {

std::error_code mfs::lock(dwarfs::file_off_t offset, size_t size)
{
  std::error_code ec;
  auto addr = reinterpret_cast<const uint8_t*>(addr_) + offset;
  if (::mlock(addr, size) != 0) {
    ec.assign(errno, boost::system::generic_category());
  }
  return ec;
}

std::error_code mfs::release(dwarfs::file_off_t offset, size_t size)
{
  std::error_code ec;
  auto misalign = offset % page_size_;

  offset -= misalign;
  size += misalign;
  size -= size % page_size_;

  auto addr = reinterpret_cast<const uint8_t*>(addr_) + offset;
  if (::munlock(addr, size) != 0) {
    ec.assign(errno, boost::system::generic_category());
  }
  return ec;
}

std::error_code mfs::release_until(dwarfs::file_off_t offset)
{
  std::error_code ec;

  offset -= offset % page_size_;

  if (::munlock(addr_, offset) != 0) {
    ec.assign(errno, boost::system::generic_category());
  }
  return ec;
}

void const* mfs::addr() const
{
  return addr_;
}

size_t mfs::size() const
{
  return size_;
}

mfs::mfs(const void* addr, size_t size) : size_(size), addr_(addr), page_size_(sysconf(_SC_PAGESIZE)) {}

std::filesystem::path const& mfs::path() const
{
  static std::filesystem::path p(TEBAKO_MOUNT_POINT);
  return p;
}

}  // namespace tebako
