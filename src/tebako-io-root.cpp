/**
 *
 * Copyright (c) 2021-2024, [Ribose Inc](https://www.ribose.com).
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

#include <tebako-pch.h>
#include <tebako-pch-pp.h>
#include <tebako-common.h>
#include <tebako-dirent.h>
#include <tebako-dfs.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-io-root.h>
#include <tebako-fd.h>
#include <tebako-mfs.h>
#include <tebako-mnt.h>

using namespace dwarfs;

namespace tebako {

struct memfs_wrapper {
  memfs* root_memfs{nullptr};
};

static folly::Synchronized<memfs_wrapper> s_root_memfs;

template <typename Functor, class... Args>
int root_memfs_call(Functor&& fn, Args&&... args)
{
  int ret = DWARFS_IO_ERROR;

  auto wrapped_memfs = s_root_memfs.rlock();

  if (wrapped_memfs->root_memfs == nullptr) {
    TEBAKO_SET_LAST_ERROR(ENOENT);
  }
  else {
    ret = (wrapped_memfs->root_memfs->*fn)(std::forward<Args>(args)...);
  }
  return ret;
}

static void release_memfs_resources(void)
{
#if defined(TEBAKO_HAS_OPENDIR) || defined(RB_W32)
  sync_tebako_dstable::get_tebako_dstable().close_all();
#endif
  sync_tebako_fdtable::get_tebako_fdtable().close_all();
  sync_tebako_mount_table::get_tebako_mount_table().clear();
  tebako_drop_cwd();
}

int mount_root_memfs(const void* data,
                     const unsigned int size,
                     const char* debuglevel,
                     const char* cachesize,
                     const char* workers,
                     const char* mlock,
                     const char* decompress_ratio,
                     const char* image_offset)
{
  int ret = 1;
  std::set_terminate([]() {
    std::cerr << "Unhandled exception" << std::endl;
    std::abort();
  });

  try {
    memfs::set_debuglevel(debuglevel);
    LOG_PROXY(debug_logger_policy, memfs::logger());
    LOG_INFO << PRJ_NAME << " version " << PRJ_VERSION_STRING;

    try {
      memfs::set_cachesize(cachesize);
      memfs::set_decompress_ratio(decompress_ratio);
      memfs::set_lock_mode(mlock);
      memfs::set_workers(workers);

      auto wrapped_memfs = s_root_memfs.wlock();

      if (wrapped_memfs->root_memfs != nullptr) {
        release_memfs_resources();
        delete wrapped_memfs->root_memfs;
      }

      wrapped_memfs->root_memfs = new memfs(data, size);
      if (wrapped_memfs->root_memfs != nullptr) {
        ret = wrapped_memfs->root_memfs->load(image_offset);
        if (ret == 0) {
          tebako_init_cwd(memfs::logger(), memfs::options().debuglevel >= logger::DEBUG);
        }
        else {
          delete wrapped_memfs->root_memfs;
          wrapped_memfs->root_memfs = nullptr;
        }
      }
      else {
        LOG_ERROR << "Failed to create memfs in " << __func__;
      }
    }
    catch (std::exception& e) {
      LOG_ERROR << "Error: " << e.what();
    }
    catch (...) {
      LOG_ERROR << "Unexpected exception";
    }
  }
  catch (...) {
    std::cerr << "Failed to initialize logger in " << __func__ << std::endl;
  }

  return ret;
}

void unmount_root_memfs(void)
{
  release_memfs_resources();
  auto wrapped_memfs = s_root_memfs.wlock();
  if (wrapped_memfs->root_memfs != nullptr) {
    delete wrapped_memfs->root_memfs;
    wrapped_memfs->root_memfs = nullptr;
  }
}

int dwarfs_access(const std::string& path, int amode, uid_t uid, gid_t gid, std::string& lnk) noexcept
{
  return root_memfs_call(&tebako::memfs::access, path, amode, uid, gid, lnk);
}

int dwarfs_lstat(const std::string& path, struct stat* buf, std::string& lnk) noexcept
{
  return root_memfs_call(&tebako::memfs::lstat, path, buf, lnk);
}

int dwarfs_readlink(const std::string& path, std::string& link, std::string& lnk) noexcept
{
  return root_memfs_call(&tebako::memfs::readlink, path, link, lnk);
}
int dwarfs_stat(const std::string& path, struct stat* buf, std::string& lnk, bool follow) noexcept
{
  return root_memfs_call(&tebako::memfs::stat, path, buf, lnk, follow);
}

int dwarfs_inode_access(uint32_t inode, int amode, uid_t uid, gid_t gid) noexcept
{
  return root_memfs_call(&tebako::memfs::inode_access, inode, amode, uid, gid);
}

int dwarfs_relative_stat(const std::string& path, struct stat* st, std::string& lnk, bool follow) noexcept
{
  return root_memfs_call(&tebako::memfs::relative_stat, path, st, lnk, follow);
}

int dwarfs_inode_relative_stat(uint32_t inode,
                               const std::string& path,
                               struct stat* buf,
                               std::string& lnk,
                               bool follow) noexcept
{
  return root_memfs_call(&tebako::memfs::inode_relative_stat, inode, path, buf, lnk, follow);
}
ssize_t dwarfs_inode_read(uint32_t inode, void* buf, size_t size, off_t offset) noexcept
{
  return root_memfs_call(&tebako::memfs::inode_read, inode, buf, size, offset);
}
int dwarfs_inode_readdir(uint32_t inode,
                         tebako::tebako_dirent* cache,
                         off_t cache_start,
                         size_t buffer_size,
                         size_t& cache_size,
                         size_t& dir_size) noexcept
{
  return root_memfs_call(&tebako::memfs::inode_readdir, inode, cache, cache_start, buffer_size, cache_size, dir_size);
}

}  // namespace tebako

#ifdef __cplusplus
extern "C" {
#endif  // !__cplusplus
int mount_root_memfs(const void* data,
                     const unsigned int size,
                     const char* debuglevel,
                     const char* cachesize,
                     const char* workers,
                     const char* mlock,
                     const char* decompress_ratio,
                     const char* image_offset)
{
  return tebako::mount_root_memfs(data, size, debuglevel, cachesize, workers, mlock, decompress_ratio, image_offset);
}

void unmount_root_memfs(void)
{
  tebako::unmount_root_memfs();
}
#ifdef __cplusplus
}
#endif  // !__cplusplus
