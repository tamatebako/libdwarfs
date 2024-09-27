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
#include <tebako-fd.h>
#include <tebako-mfs.h>
#include <tebako-mnt.h>

namespace stdfs = std::filesystem;

namespace dwarfs {

template <typename LoggerPolicy>
void load_filesystem(dwarfs_userdata& userdata)
{
  LOG_PROXY(LoggerPolicy, userdata.lgr);
  auto ti = LOG_TIMED_INFO;
  auto& opts = userdata.opts;

  filesystem_options fsopts;
  fsopts.lock_mode = opts.lock_mode;
  fsopts.block_cache.max_bytes = opts.cachesize;
  fsopts.block_cache.num_workers = opts.workers;
  fsopts.block_cache.decompress_ratio = opts.decompress_ratio;
  fsopts.block_cache.mm_release = !opts.cache_image;
  fsopts.block_cache.init_workers = true;
  fsopts.metadata.enable_nlink = bool(opts.enable_nlink);
  fsopts.metadata.readonly = bool(opts.readonly);

  if (opts.image_offset_str) {
    std::string image_offset{opts.image_offset_str};

    try {
      fsopts.image_offset =
          image_offset == "auto" ? filesystem_options::IMAGE_OFFSET_AUTO : folly::to<file_off_t>(image_offset);
    }
    catch (...) {
      DWARFS_THROW(runtime_error, "failed to parse offset: " + image_offset);
    }
  }

  constexpr int inode_offset =
#ifdef FUSE_ROOT_ID
      FUSE_ROOT_ID
#else
      0
#endif
      ;

  std::unordered_set<std::string> perfmon_enabled;
#if DWARFS_PERFMON_ENABLED
  if (opts.perfmon_enabled_str) {
    folly::splitTo<std::string>(',', opts.perfmon_enabled_str, std::inserter(perfmon_enabled, perfmon_enabled.begin()));
  }
#endif

  userdata.perfmon = nullptr;
  userdata.fs = filesystem_v2(userdata.lgr, std::make_shared<tebako::mfs>(userdata.data, userdata.size), fsopts,
                              inode_offset, userdata.perfmon);

  ti << "file system initialized";
}

}  // namespace dwarfs

using namespace dwarfs;
using namespace tebako;

// dwarFS user data including fs pointer
// RW lock implemented using folly tooling
// github.com/facebook/folly/blob/master/folly/docs/Synchronized
static folly::Synchronized<dwarfs_userdata*> usd{NULL};
uint32_t dwarfs_root = 0;

// Drop previously loaded dwarFS image
void drop_fs(void)
{
  {
    auto locked = usd.wlock();
    if (*locked) {
      delete *locked;
    }
    *locked = NULL;
  }
#if defined(TEBAKO_HAS_OPENDIR) || defined(RB_W32)
  sync_tebako_dstable::get_tebako_dstable().close_all();
#endif
  sync_tebako_fdtable::get_tebako_fdtable().close_all();
  tebako_drop_cwd();
}

// Loads dwarFS image
// ["C" wrapper for load_filesystem]
int load_fs(const void* data,
            const unsigned int size,
            const char* debuglevel,
            const char* cachesize,
            const char* workers,
            const char* mlock,
            const char* decompress_ratio,
            const char* image_offset)
{
  std::set_terminate([]() {
    std::cout << "Unhandled exception" << std::endl;
    std::abort();
  });

  try {
    drop_fs();

    auto locked = usd.wlock();
    *locked = new dwarfs_userdata(std::cerr, data, size);
    auto p = *locked;

    p->opts.cache_image = 0;
    p->opts.cache_files = 1;

    p->opts.debuglevel = debuglevel ? logger::parse_level(debuglevel) : logger::INFO;

    p->lgr.set_threshold(p->opts.debuglevel);
    p->lgr.set_with_context(p->opts.debuglevel >= logger::DEBUG);

    p->opts.cachesize = cachesize ? dwarfs::parse_size_with_unit(cachesize) : (static_cast<size_t>(512) << 20);
    p->opts.workers = workers ? folly::to<size_t>(workers) : 2;
    p->opts.lock_mode = mlock ? parse_mlock_mode(mlock) : mlock_mode::NONE;
    p->opts.decompress_ratio = decompress_ratio ? folly::to<double>(decompress_ratio) : 0.8;

    if (p->opts.decompress_ratio < 0.0 || p->opts.decompress_ratio > 1.0) {
      std::cerr << "error: decratio must be between 0.0 and 1.0" << std::endl;
      return 1;
    }

    p->opts.image_offset_str = image_offset;

    LOG_PROXY(debug_logger_policy, p->lgr);
    LOG_INFO << PRJ_NAME << " version " << PRJ_VERSION_STRING;

    tebako_init_cwd(p->lgr, p->opts.debuglevel >= logger::DEBUG);

    (p->opts.debuglevel >= logger::DEBUG) ? load_filesystem<debug_logger_policy>(*p)
                                          : load_filesystem<prod_logger_policy>(*p);

    auto dyn = p->fs.metadata_as_dynamic();
    dwarfs_root = dyn["root"]["inode"].asInt();
  }

  catch (stdfs::filesystem_error const& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  catch (std::exception const& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  }
  catch (...) {
    return DWARFS_IO_ERROR;
  }

  return DWARFS_IO_CONTINUE;
}

// dwarfs_process_link
//  Handles relative link
// params
//  lnk - symlink to process
//  p_iterator - iterator to the path, rebuild with symlink applied
//  p_path - path under traverse, rebuild with symlink applied
//
// returns
//  DWARFS_IO_CONTINUE - success [st is filled]
//  DWARFS_IO_ERROR - error [errno is set]
//  DWARFS_LINK - symlink or mount point  [lnk is set]

static int dwarfs_process_link(std::string& lnk, stdfs::path::iterator& p_iterator, stdfs::path& p_path)
{
  stdfs::path new_path = stdfs::path(lnk);
  for (auto it = p_iterator; it != p_path.end(); ++it) {
    new_path /= it->string();
  }
  new_path = new_path.lexically_normal();
  lnk = new_path.generic_string();
  int ret = new_path.is_relative() ? DWARFS_S_LINK_RELATIVE : DWARFS_S_LINK_ABSOLUTE;
  if (ret == DWARFS_S_LINK_RELATIVE) {
    p_path = std::move(new_path);
    p_iterator = p_path.begin();
  }
  return ret;
}

// dwarfs_process_inode
//  Processes inode and handles relative links
// params
//  fs - dwarfs filesystem
//  pi - inode to process
//  st - out parameter to store the stat structure
//  follow - should we follow the last element in the path if ti is symlink
//  lnk - out parameter to store the symlink (or mount point)
//  p_iterator - iterator to the path
//  p_path - path under traverse
//
// returns
//  DWARFS_IO_CONTINUE - success [st is filled]
//  DWARFS_IO_ERROR - error [errno is set]
//  DWARFS_LINK - symlink or mount point  [lnk is set]

static int dwarfs_process_inode(filesystem_v2& fs,
                                inode_view& pi,
                                dwarfs::file_stat* st,
                                bool follow,
                                std::string& lnk,
                                // Using  stdfs::path::iterator& below
                                // causes std::bad_alloc at ++p_iterator
                                // at least for gcc 12 headers. May be others as well.
                                stdfs::path::iterator& p_iterator,
                                stdfs::path& p_path)
{
  int ret = DWARFS_IO_CONTINUE;
  int err = fs.getattr(pi, st);
  if (err == 0) {
    // (1) It is symlink
    // (2a) It is not the last element in the path
    // (2b)   or we should follow the last element  (lstat called)
    if (S_ISLNK(st->mode) && (++p_iterator != p_path.end() || follow)) {
      err = fs.readlink(pi, &lnk);
      if (err == 0) {
        ret = dwarfs_process_link(lnk, p_iterator, p_path);
      }
    }
  }
  // Failed to get inode attributes
  // dwarfs returns (-errno)
  if (err != 0) {
    TEBAKO_SET_LAST_ERROR(-err);
    ret = DWARFS_IO_ERROR;
  }
  return ret;
}

// *** Now this is the core function ***
//
// dwarfs_find_inode
//   Finds inode
//   Converts mount points to links
//   Follows relative links
//
// params
//  start_from - inode number to start from
//  p_path - path to find
//  follow - should we follow the last element in the path if ti is symlink
//  lnk - out parameter to store the symlink (or mount point)
//  st - out parameter to store the stat structure
//
// returns
//  DWARFS_IO_CONTINUE - success [st is filled]
//  DWARFS_IO_ERROR - error [errno is set]
//  DWARFS_LINK - symlink or mount point  [lnk is set]

static int dwarfs_find_inode(uint32_t start_from,
                             const stdfs::path& path,
                             bool follow_last,
                             std::string& lnk,
                             struct stat* st) noexcept
{
  int ret = DWARFS_IO_CONTINUE;
  dwarfs::file_stat dwarfs_st;
  stdfs::path p_path{path};  // a copy of the path, mangled if symlink is found

  auto locked = usd.rlock();
  auto p = *locked;

  try {
    if (p) {
      LOG_PROXY(debug_logger_policy, p->lgr);
      LOG_DEBUG << __func__ << " [ @inode:" << start_from << " path:" << path << " ]";

      auto pi = p->fs.find(start_from);
      auto p_iterator = p_path.begin();
      auto m_table = sync_tebako_mount_table::get_tebako_mount_table();

      if (pi) {
        ret = dwarfs_process_inode(p->fs, *pi, &dwarfs_st, follow_last, lnk, p_iterator, p_path);
        while (p_iterator != p_path.end() && p_iterator->string() != "" && ret == DWARFS_IO_CONTINUE) {
          auto inode = pi->inode_num();
          auto mount_point = m_table.get(inode, p_iterator->string());
          // Hit mount point
          // Convert it to symlink and proceed
          if (mount_point) {
            lnk = *mount_point;
            LOG_DEBUG << __func__ << " [ mount point --> \"" << lnk << "\" ]";
            ret = dwarfs_process_link(lnk, ++p_iterator, p_path);
            if (ret == DWARFS_S_LINK_RELATIVE) {
              ret = DWARFS_IO_CONTINUE;
              continue;
            }
          }
          else {
            auto pi_prev = pi;
            pi = p->fs.find(inode, p_iterator->string().c_str());

            if (pi) {
              ret = dwarfs_process_inode(p->fs, *pi, &dwarfs_st, follow_last, lnk, p_iterator, p_path);
              if (ret == DWARFS_S_LINK_RELATIVE || ret == DWARFS_S_LINK_ABSOLUTE) {
                LOG_DEBUG << __func__ << " [ reparse point --> \"" << lnk << "\" ]";
              }
              if (ret == DWARFS_S_LINK_RELATIVE) {
                pi = pi_prev;
                ret = DWARFS_IO_CONTINUE;
                continue;
              }
            }
            // Failed to find the next element in the path
            else {
              TEBAKO_SET_LAST_ERROR(ENOENT);
              ret = DWARFS_IO_ERROR;
            }
          }
          if (p_iterator != p_path.end()) {
            ++p_iterator;
          }
        }
      }
      // Failed to find the start inode
      else {
        TEBAKO_SET_LAST_ERROR(ENOENT);
        ret = DWARFS_IO_ERROR;
      }
      // Copy the stat structure only if there is no error
      if (ret != DWARFS_IO_ERROR) {
#if defined(_WIN32)
        copy_file_stat<false>(st, dwarfs_st);
#else
        copy_file_stat<true>(st, dwarfs_st);
#endif
      }
    }
    // Filesystem not mounted
    else {
      TEBAKO_SET_LAST_ERROR(ENOENT);
      ret = DWARFS_IO_ERROR;
    }
  }
  catch (dwarfs::system_error const& e) {
    TEBAKO_SET_LAST_ERROR(e.get_errno());
    ret = DWARFS_IO_ERROR;
  }
  catch (...) {
    ret = DWARFS_IO_ERROR;
    TEBAKO_SET_LAST_ERROR(ENOMEM);
  }
  return ret;
}

// dwarfs_find_inode_abs
// Finds inode and follows absolute link if it is within memfs
//
// params
//  start_from - inode number to start from
//  p_path - path to find
//  follow - should we follow the last element in the path if ti is symlink
//  lnk - out parameter to store the symlink (or mount point)
//  st - out parameter to store the stat structure
//
// returns
//  DWARFS_IO_CONTINUE - success [st is filled]
//  DWARFS_IO_ERROR - error [errno is set]
//  DWARFS_LINK - symlink or mount point  [lnk is set]

static int dwarfs_find_inode_abs(uint32_t start_from,
                                 const stdfs::path& path,
                                 bool follow,
                                 std::string& lnk,
                                 struct stat* st) noexcept
{
  int ret = DWARFS_IO_ERROR;
  try {
    ret = dwarfs_find_inode(start_from, path, follow, lnk, st);
    // Follow absolute links if necessary
    // (indirect recursion)
    if (ret == DWARFS_S_LINK_ABSOLUTE) {
      if (is_tebako_path(lnk.c_str())) {
        if (follow) {
#ifdef RB_W32
          struct STAT_TYPE _st;
          ret = tebako_stat(lnk.c_str(), &_st);
          st << _st;
#else
          ret = tebako_stat(lnk.c_str(), st);
#endif
        }
        else {
          ret = DWARFS_IO_CONTINUE;
        }
      }
      else {
        ret = DWARFS_S_LINK_OUTSIDE;
      }
    }
  }
  catch (dwarfs::system_error const& e) {
    TEBAKO_SET_LAST_ERROR(e.get_errno());
    ret = DWARFS_IO_ERROR;
  }
  catch (...) {
    ret = DWARFS_IO_ERROR;
    TEBAKO_SET_LAST_ERROR(ENOMEM);
  }
  return ret;
}

// dwarfs_find_inode_root
// Finds inode startinf from memfs root
//
// params
//  path - path to find
//  follow - should we follow the last element in the path if ti is symlink
//  lnk - out parameter to store the symlink (or mount point)
//  st - out parameter to store the stat structure
//
// returns
//  DWARFS_IO_CONTINUE - success [st is filled]
//  DWARFS_IO_ERROR - error [errno is set]
//  DWARFS_LINK - symlink or mount point  [lnk is set]

static int dwarfs_find_inode_root(const std::string& path, bool follow, std::string& lnk, struct stat* st) noexcept
{
  // Normally we remove '/__tebako_memfs__/'
  // However, there is also a case when it is memfs root and path isn just
  // '/__tebako_memfs__'
  auto adjusted_path =
      path.substr(path[TEBAKO_MOUNT_POINT_LENGTH] == '\0' ? TEBAKO_MOUNT_POINT_LENGTH : TEBAKO_MOUNT_POINT_LENGTH + 1);

  return dwarfs_find_inode_abs(dwarfs_root, adjusted_path, follow, lnk, st);
}

template <typename Functor, class... Args>
int safe_dwarfs_call(Functor&& fn, const char* caller, uint32_t inode, Args&&... args)
{
  int ret = DWARFS_IO_ERROR;
  int err = ENOENT;
  auto locked = usd.rlock();
  auto p = *locked;
  if (p) {
    if (p->opts.debuglevel >= logger::DEBUG) {
      LOG_PROXY(debug_logger_policy, p->lgr);
      LOG_DEBUG << caller << " [ " << inode << " ]";
    }
    try {
      ret = fn(&p->fs, inode, std::forward<Args>(args)...);
      if (ret < 0) {
        err = ret;
        ret = DWARFS_IO_ERROR;
      }
    }
    catch (dwarfs::system_error const& e) {
      err = e.get_errno();
      ret = DWARFS_IO_ERROR;
    }
    catch (...) {
      err = EIO;
      ret = DWARFS_IO_ERROR;
    }
  }
  if (ret < DWARFS_IO_CONTINUE) {
    TEBAKO_SET_LAST_ERROR(err < 0 ? -err : err);
  }
  return ret;
}

// We are testing against owner permissions
static int dwarfs_access_inner(int amode, struct stat* st)
{
  int ret = DWARFS_IO_CONTINUE;
// WIN32 ?????????
#ifdef _WIN32
  if (amode & W_OK) {
#else
  if ((!(st->st_mode & S_IRUSR) && (amode & R_OK)) || (amode & W_OK) || (!(st->st_mode & S_IXUSR) && (amode & X_OK))) {
#endif
    ret = DWARFS_IO_ERROR;
    TEBAKO_SET_LAST_ERROR(EACCES);
  }
  return ret;
}

int dwarfs_access(const std::string& path, int amode, uid_t uid, gid_t gid, std::string& lnk) noexcept
{
  struct stat st;
  int ret = dwarfs_stat(path, &st, lnk, true);
  if (ret == DWARFS_IO_CONTINUE) {
    ret = dwarfs_access_inner(amode, &st);
  }
  return ret;
}

static int get_dwarfs_file_stat(filesystem_v2* fs, inode_view& inode, struct stat* st)
{
  dwarfs::file_stat dwarfs_file_stat;
  int ret = fs->getattr(inode, &dwarfs_file_stat);
#if defined(_WIN32)
  copy_file_stat<false>(st, dwarfs_file_stat);
#else
  copy_file_stat<true>(st, dwarfs_file_stat);
#endif
  return ret;
}

int dwarfs_inode_readlink(uint32_t inode, std::string& lnk) noexcept
{
  int ret = safe_dwarfs_call(
      [](filesystem_v2* fs, uint32_t inode, std::string& lnk) {
        auto pi = fs->find(inode);
        return pi ? fs->readlink(*pi, &lnk) : DWARFS_IO_ERROR;
      },
      __func__, inode, lnk);

  return ret;
}

int dwarfs_inode_access(uint32_t inode, int amode, uid_t uid, gid_t gid) noexcept
{
  return safe_dwarfs_call(
      [](filesystem_v2* fs, uint32_t inode, int amode, uid_t uid, gid_t gid) {
        int ret = DWARFS_IO_ERROR;
        auto pi = fs->find(inode);
        if (pi) {
          struct stat st;
          ret = get_dwarfs_file_stat(fs, *pi, &st);
          if (ret == DWARFS_IO_CONTINUE) {
            ret = dwarfs_access_inner(amode, &st);
          }
        }
        else {
          TEBAKO_SET_LAST_ERROR(ENOENT);
        }
        return ret;
      },
      __func__, inode, amode, uid, gid);
}

ssize_t dwarfs_inode_read(uint32_t inode, void* buf, size_t size, off_t offset) noexcept
{
  return safe_dwarfs_call([](filesystem_v2* fs, uint32_t inode, void* buf, size_t size,
                             off_t offset) { return fs->read(inode, static_cast<char*>(buf), size, offset); },
                          __func__, inode, buf, size, offset);
}

static int internal_readdir(filesystem_v2* fs,
                            uint32_t inode,
                            tebako_dirent* cache,
                            off_t cache_start,
                            size_t buffer_size,
                            size_t& cache_size,
                            size_t& dir_size)
{
  int ret = -1;
  auto pi = fs->find(inode);
  if (pi) {
    auto dir = fs->opendir(*pi);
    if (dir) {
      dir_size = fs->dirsize(*dir);
      struct stat st;
      cache_size = 0;
      bool pOK = true;
      while (cache_start + cache_size < dir_size && cache_size < buffer_size && pOK) {
        auto res = fs->readdir(*dir, cache_start + cache_size);
        if (!res) {
          pOK = false;
        }
        else {
          auto [entry, name_view] = *res;
          std::string name(std::move(name_view));
          struct stat st;
          get_dwarfs_file_stat(fs, entry, &st);

#ifndef RB_W32
          cache[cache_size].e.d_ino = st.st_ino;
#if __MACH__
          cache[cache_size].e.d_seekoff = cache_start + cache_size;
#else
          cache[cache_size].e.d_off = cache_start + cache_size;
#endif
          cache[cache_size].e.d_type = IFTODT(st.st_mode);
          strncpy(cache[cache_size]._e.d_name, name.c_str(), TEBAKO_PATH_LENGTH);
          cache[cache_size]._e.d_name[TEBAKO_PATH_LENGTH] = '\0';
          cache[cache_size].e.d_reclen = sizeof(cache[0]);
#else
#ifdef _WIN32
          cache[cache_size].e.d_altname = 0;
          cache[cache_size].e.d_altlen = 0;
          cache[cache_size].e.d_name = cache[cache_size].d_name;
          if (S_ISDIR(st.st_mode)) {
            cache[cache_size].e.d_type = DT_DIR;
          }
          else if (S_ISLNK(st.st_mode)) {
            cache[cache_size].e.d_type = DT_LNK;
          }
          else {
            cache[cache_size].e.d_type = DT_REG;
          }
          cache[cache_size].e.d_namlen = std::min(name.length(), TEBAKO_PATH_LENGTH - 1);
#else
          cache[cache_size].e.d_reclen = 0;
          cache[cache_size].e.d_namlen = std::min(name.length(), sizeof(cache[cache_size].e.d_name) - 1);
#endif
          static int dummy = INT_MAX;
          cache[cache_size].e.d_ino = dummy--;
          strncpy(cache[cache_size].e.d_name, name.c_str(), cache[cache_size].e.d_namlen);
          cache[cache_size].e.d_name[cache[cache_size].e.d_namlen] = '\0';
#endif
          ++cache_size;
        }
      }
      if (pOK) {
        ret = 0;
      }
    }
  }
  if (ret < 0) {
    TEBAKO_SET_LAST_ERROR(ENOTDIR);
    cache_size = 0;
  }
  return ret;
}

int dwarfs_inode_readdir(uint32_t inode,
                         tebako_dirent* cache,
                         off_t cache_start,
                         size_t buffer_size,
                         size_t& cache_size,
                         size_t& dir_size) noexcept
{
  return safe_dwarfs_call(
      [](filesystem_v2* fs, uint32_t inode, tebako_dirent* cache, off_t cache_start, size_t buffer_size,
         size_t& cache_size, size_t& dir_size) {
        return internal_readdir(fs, inode, cache, cache_start, buffer_size, cache_size, dir_size);
      },
      __func__, inode, cache, cache_start, buffer_size, cache_size, dir_size);
}

int dwarfs_stat(const std::string& path, struct stat* st, std::string& lnk, bool follow) noexcept
{
  return dwarfs_find_inode_root(path, follow, lnk, st);
}

int dwarfs_inode_relative_stat(uint32_t inode,
                               const std::string& path,
                               struct stat* st,
                               std::string& lnk,
                               bool follow) noexcept
{
  return dwarfs_find_inode_abs(inode, path, follow, lnk, st);
}

#if defined(TEBAKO_HAS_LSTAT) || defined(RB_W32) || defined(_WIN32)
int dwarfs_lstat(const std::string& path, struct stat* st, std::string& lnk) noexcept
{
  return dwarfs_find_inode_root(path, false, lnk, st);
}
#endif

int dwarfs_readlink(const std::string& path, std::string& link, std::string& lnk) noexcept
{
  struct stat st;
  int ret = dwarfs_find_inode_root(path, false, lnk, &st);
  if (ret == DWARFS_IO_CONTINUE) {
    ret = dwarfs_inode_readlink(st.st_ino, link);
  }
  return ret;
}
