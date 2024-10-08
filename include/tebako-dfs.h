/**
 *
 * Copyright (c) 2021-2024 [Ribose Inc](https://www.ribose.com).
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

#include "dwarfs/error.h"
#include "dwarfs/file_stat.h"
#include "dwarfs/filesystem_v2.h"
#include "dwarfs/fstypes.h"
#include "dwarfs/metadata_v2.h"
#include "dwarfs/mmap.h"
#include "dwarfs/performance_monitor.h"
#include "dwarfs/options.h"
#include "dwarfs/util.h"

void tebako_init_cwd(dwarfs::logger& lgr, bool need_debug_policy);
void tebako_drop_cwd(void);

namespace dwarfs {

struct options {
  std::filesystem::path progname;
  std::filesystem::path fsimage;
  int seen_mountpoint{0};
  char const* cachesize_str{nullptr};
  char const* debuglevel_str{nullptr};
  char const* workers_str{nullptr};
  char const* mlock_str{nullptr};
  char const* decompress_ratio_str{nullptr};
  char const* image_offset_str{nullptr};
  char const* cache_tidy_strategy_str{nullptr};
  char const* cache_tidy_interval_str{nullptr};
  char const* cache_tidy_max_age_str{nullptr};
#if DWARFS_PERFMON_ENABLED
  char const* perfmon_enabled_str{nullptr};
#endif
  int enable_nlink{0};
  int readonly{0};
  int cache_image{0};
  int cache_files{0};
  size_t cachesize{0};
  size_t workers{0};
  mlock_mode lock_mode{mlock_mode::NONE};
  double decompress_ratio{0.0};
  logger::level_type debuglevel{logger::level_type::ERROR};
  cache_tidy_strategy block_cache_tidy_strategy{cache_tidy_strategy::NONE};
  std::chrono::milliseconds block_cache_tidy_interval{std::chrono::minutes(5)};
  std::chrono::milliseconds block_cache_tidy_max_age{std::chrono::minutes{10}};
};

struct dwarfs_userdata {
  dwarfs_userdata(std::ostream& os, const void* dt, const unsigned int sz) : lgr{os}, data{dt}, size{sz} {}

  const void* data;
  const unsigned int size;

  dwarfs::options opts;
  stream_logger lgr;
  filesystem_v2 fs;

  std::shared_ptr<performance_monitor> perfmon;
};
}  // namespace dwarfs
