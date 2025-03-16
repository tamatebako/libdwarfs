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
#include <tebako-memfs.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-mfs.h>
#include <tebako-memfs-table.h>
#include <tebako-mount-table.h>

using namespace dwarfs;

namespace tebako {

filesystem_options& operator<<(filesystem_options& fsopts, tebako::memfs_options& opts)
{
  fsopts.lock_mode = opts.lock_mode;
  fsopts.block_cache.max_bytes = opts.cachesize;
  fsopts.block_cache.num_workers = opts.workers;
  fsopts.block_cache.decompress_ratio = opts.decompress_ratio;
  fsopts.block_cache.mm_release = !opts.cache_image;
  fsopts.block_cache.init_workers = true;
  fsopts.metadata.enable_nlink = bool(opts.enable_nlink);
  fsopts.metadata.readonly = bool(opts.readonly);
  return fsopts;
}

stream_logger& memfs::logger()
{
  static stream_logger lgr{std::cerr};
  return lgr;
}

memfs_options& memfs::options()
{
  static memfs_options opts;
  return opts;
}

memfs::memfs(const void* dt, const unsigned int sz, uint32_t df_root_inode)
    : data{dt}, size{sz}, dwarfs_root_inode(df_root_inode)
{
  fsopts << options();
}

int memfs::load(const char* image_offset)
{
  LOG_PROXY(debug_logger_policy, logger());

  try {
    set_image_offset_str(image_offset);
    fs = filesystem_v2(logger(), std::make_shared<tebako::mfs>(data, size), fsopts, dwarfs_root_inode, nullptr);
    LOG_TIMED_INFO << "Filesystem initialized";
  }

  catch (stdfs::filesystem_error const& e) {
    LOG_ERROR << "Filesystem error: " << e.what();
    return -1;
  }
  catch (std::exception const& e) {
    LOG_ERROR << "Error: " << e.what();
    return -1;
  }
  catch (...) {
    LOG_ERROR << "Unexpected error";
    return -1;
  }

  return 0;
}

void memfs::set_cachesize(const char* cachesize)
{
  options().cachesize = (cachesize != nullptr) ? parse_size_with_unit(cachesize) : (static_cast<size_t>(512) << 20);
}

void memfs::set_debuglevel(const char* debuglevel)
{
  options().debuglevel = (debuglevel != nullptr) ? logger::parse_level(debuglevel) : logger::INFO;
  logger().set_threshold(options().debuglevel);
  logger().set_with_context(options().debuglevel >= logger::DEBUG);
}

void memfs::set_decompress_ratio(const char* decompress_ratio)
{
  options().decompress_ratio = (decompress_ratio != nullptr) ? folly::to<double>(decompress_ratio) : 0.8;
  if (options().decompress_ratio < 0.0 || options().decompress_ratio > 1.0) {
    DWARFS_THROW(runtime_error, std::string("decratio must be between 0.0 and 1.0; got ") + decompress_ratio);
  }
}

void memfs::set_image_offset_str(const char* image_offset_str)
{
  if (image_offset_str) {
    std::string image_offset{image_offset_str};
    try {
      fsopts.image_offset =
          image_offset == "auto" ? filesystem_options::IMAGE_OFFSET_AUTO : folly::to<file_off_t>(image_offset);
    }
    catch (...) {
      DWARFS_THROW(runtime_error, "failed to parse offset: " + image_offset);
    }
  }
}

void memfs::set_lock_mode(const char* mlock)
{
  options().lock_mode = (mlock != nullptr) ? parse_mlock_mode(mlock) : mlock_mode::NONE;
}

void memfs::set_workers(const char* workers)
{
  options().workers = (workers != nullptr) ? folly::to<size_t>(workers) : 2;
}

// *** Now this is the core function ***
//
// memfs::find_inode
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

int memfs::find_inode(uint32_t start_from,
                      const stdfs::path& path,
                      bool follow_last,
                      std::string& lnk,
                      struct stat* st) noexcept
{
  int ret = DWARFS_IO_CONTINUE;
  dwarfs::file_stat dwarfs_st;
  stdfs::path p_path{path};  // a copy of the path, mangled if symlink is found

  try {
    LOG_PROXY(debug_logger_policy, logger());
    LOG_DEBUG << __func__ << " [ @inode:" << start_from << " path:" << path << " ]";

    auto pi = fs.find(start_from);
    auto p_iterator = p_path.begin();
    auto m_table = sync_tebako_mount_table::get_tebako_mount_table();

    if (pi) {
      ret = process_inode(*pi, &dwarfs_st, follow_last, lnk, p_iterator, p_path);
      while (p_iterator != p_path.end() && !p_iterator->empty() && ret == DWARFS_IO_CONTINUE) {
        auto inode = pi->inode_num() + get_root_inode();
        auto mount_point = m_table.get(inode, p_iterator->string());
        // Hit mount point
        // Convert it to symlink and proceed
        if (mount_point) {
          if (std::holds_alternative<std::string>(*mount_point)) {
            lnk = std::get<std::string>(*mount_point);
            LOG_DEBUG << __func__ << " [ mount point --> \"" << lnk << "\" ]";
            ret = process_link(lnk, ++p_iterator, p_path);
            if (ret == DWARFS_S_LINK_RELATIVE) {
              ret = DWARFS_IO_CONTINUE;
              continue;
            }
          }
          else if (std::holds_alternative<uint32_t>(*mount_point)) {
            uint32_t index = std::get<uint32_t>(*mount_point);
            LOG_DEBUG << __func__ << " [ mount point --> memfs:\"" << index << "\" ]";
            auto next_memfs = tebako::sync_tebako_memfs_table::get_tebako_memfs_table().get(index);
            if (next_memfs != nullptr) {
              auto next_inode = next_memfs->get_root_inode();
              stdfs::path next_path = stdfs::path("");
              while (++p_iterator != p_path.end()) {
                next_path /= p_iterator->string();
              }
              return next_memfs->find_inode(next_inode, next_path, follow_last, lnk, st);
            }
            else {
              LOG_DEBUG << __func__ << " [ Memfs not mounted ]";
              TEBAKO_SET_LAST_ERROR(ENOENT);
              ret = DWARFS_IO_ERROR;
            }
          }
          else {
            LOG_DEBUG << __func__ << " [ Invalid mount point type ]";
            TEBAKO_SET_LAST_ERROR(ENOENT);
            ret = DWARFS_IO_ERROR;
          }
        }
        else {
          auto pi_prev = pi;
          pi = fs.find(inode, p_iterator->string().c_str());

          if (pi) {
            ret = process_inode(*pi, &dwarfs_st, follow_last, lnk, p_iterator, p_path);
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

// memfs::find_inode_abs
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

int memfs::find_inode_abs(uint32_t start_from,
                          const stdfs::path& path,
                          bool follow,
                          std::string& lnk,
                          struct stat* st) noexcept
{
  int ret = DWARFS_IO_ERROR;
  try {
    ret = find_inode(start_from, path, follow, lnk, st);
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

// memfs::find_inode_root
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

int memfs::find_inode_root(const std::string& path, bool follow, std::string& lnk, struct stat* st) noexcept
{
  // Normally we remove '/__tebako_memfs__/'
  // However, there is also a case when it is memfs root and path isn just
  // '/__tebako_memfs__'
  int ret = DWARFS_IO_ERROR;
  if (path.length() < TEBAKO_MOUNT_POINT_LENGTH) {
    TEBAKO_SET_LAST_ERROR(ENOENT);
  }
  else {
    auto adjusted_path = path.substr(path[TEBAKO_MOUNT_POINT_LENGTH] == '\0' ? TEBAKO_MOUNT_POINT_LENGTH
                                                                             : TEBAKO_MOUNT_POINT_LENGTH + 1);

    ret = find_inode_abs(dwarfs_root_inode, adjusted_path, follow, lnk, st);
  }
  return ret;
}

// memfs::process_inode
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

int memfs::process_inode(inode_view& pi,
                         dwarfs::file_stat* st,
                         bool follow,
                         std::string& lnk,
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
        ret = process_link(lnk, p_iterator, p_path);
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

// memfs::process_link
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

int memfs::process_link(std::string& lnk, stdfs::path::iterator& p_iterator, stdfs::path& p_path)
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

int memfs::readlink(const std::string& path, std::string& link, std::string& lnk) noexcept
{
  struct stat st;
  int ret = find_inode_root(path, false, lnk, &st);
  if (ret == DWARFS_IO_CONTINUE) {
    ret = inode_memfs_call(&tebako::memfs::inode_readlink, st.st_ino, link);
  }
  return ret;
}

int memfs::access(const std::string& path, int amode, uid_t uid, gid_t gid, std::string& lnk) noexcept
{
  struct stat st;
  int ret = stat(path, &st, lnk, true);
  if (ret == DWARFS_IO_CONTINUE) {
    ret = i_access(amode, &st);
  }
  return ret;
}

int memfs::dwarfs_file_stat(inode_view& inode, struct stat* st)
{
  dwarfs::file_stat dwarfs_file_stat;
  int ret = fs.getattr(inode, &dwarfs_file_stat);
#if defined(_WIN32)
  copy_file_stat<false>(st, dwarfs_file_stat);
#else
  copy_file_stat<true>(st, dwarfs_file_stat);
#endif
  if (ret < 0) {
    TEBAKO_SET_LAST_ERROR(-ret);
    ret = DWARFS_IO_ERROR;
  }
  return ret;
}

int memfs::inode_access(uint32_t inode, int amode, uid_t uid, gid_t gid) noexcept
{
  int ret = DWARFS_IO_ERROR;
  auto pi = fs.find(inode);
  if (pi) {
    struct stat st;
    ret = dwarfs_file_stat(*pi, &st);
    if (ret == DWARFS_IO_CONTINUE) {
      ret = i_access(amode, &st);
    }
  }
  else {
    TEBAKO_SET_LAST_ERROR(ENOENT);
  }
  return ret;
}

ssize_t memfs::inode_read(uint32_t inode, void* buf, size_t size, off_t offset) noexcept
{
  int ret = DWARFS_IO_ERROR;
  int err = fs.read(inode, static_cast<char*>(buf), size, offset);
  if (err < 0) {
    TEBAKO_SET_LAST_ERROR(-err);
  }
  else {
    ret = err;
  }
  return ret;
}

int memfs::inode_readdir(uint32_t inode,
                         tebako_dirent* cache,
                         off_t cache_start,
                         size_t buffer_size,
                         size_t& cache_size,
                         size_t& dir_size) noexcept
{
  int ret = -1;
  auto pi = fs.find(inode);
  if (pi) {
    auto dir = fs.opendir(*pi);
    if (dir) {
      dir_size = fs.dirsize(*dir);
      struct stat st;
      cache_size = 0;
      bool pOK = true;
      while (cache_start + cache_size < dir_size && cache_size < buffer_size && pOK) {
        auto res = fs.readdir(*dir, cache_start + cache_size);
        if (!res) {
          pOK = false;
        }
        else {
          auto [entry, name_view] = *res;
          std::string name(std::move(name_view));
          struct stat st;
          ret = dwarfs_file_stat(entry, &st);

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

int memfs::inode_readlink(uint32_t inode, std::string& lnk) noexcept
{
  int ret = DWARFS_IO_ERROR;
  auto pi = fs.find(inode);
  if (pi) {
    int err = fs.readlink(*pi, &lnk);
    if (err < 0) {
      TEBAKO_SET_LAST_ERROR(-err);
    }
    else {
      ret = err;
    }
  }
  else {
    TEBAKO_SET_LAST_ERROR(ENOENT);
  }
  return ret;
}

// We are testing against owner permissions
int memfs::i_access(int amode, struct stat* st)
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

template <typename Functor, class... Args>
int memfs::safe_dwarfs_call(Functor&& fn, const char* caller, uint32_t inode, Args&&... args)
{
  int ret = DWARFS_IO_ERROR;
  int err = ENOENT;
  if (options().debuglevel >= logger::DEBUG) {
    LOG_PROXY(debug_logger_policy, logger());
    LOG_DEBUG << caller << " [ " << inode << " ]";
  }
  try {
    ret = fn(inode, std::forward<Args>(args)...);
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
  if (ret < DWARFS_IO_CONTINUE) {
    TEBAKO_SET_LAST_ERROR(err < 0 ? -err : err);
  }
  return ret;
}
}  // namespace tebako
