/**
 *
 * Copyright (c) 2021, [Ribose Inc](https://www.ribose.com).
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

#include <array>
#include <iostream>
#include <stdexcept>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <folly/Conv.h>
#include <folly/experimental/symbolizer/SignalHandler.h>

#include "tebako-common.h"
#include "tebako-mfs.h"
#include "tebako-dfs.h"

using namespace dwarfs;

/*
*   C wrapper for dwarfs load_filesystem implementation  @ tebako-dfs.cpp
*/
extern "C" int load_fs(const unsigned char data[], const unsigned int size)
{
    int ret = 0;
    try
    {
        dwarfs_userdata userdata(std::cerr);

        userdata.opts.cache_image = 0;
        userdata.opts.cache_files = 1;


        try {
            // TODO: foreground mode, stderr vs. syslog?

            userdata.opts.debuglevel = userdata.opts.debuglevel_str
                ? logger::parse_level(userdata.opts.debuglevel_str)
                : logger::INFO;

            userdata.lgr.set_threshold(userdata.opts.debuglevel);
            userdata.lgr.set_with_context(userdata.opts.debuglevel >= logger::DEBUG);

            userdata.opts.cachesize = userdata.opts.cachesize_str ? dwarfs::parse_size_with_unit(userdata.opts.cachesize_str) : (static_cast<size_t>(512) << 20);
            userdata.opts.workers = userdata.opts.workers_str ? folly::to<size_t>(userdata.opts.workers_str) : 2;
            userdata.opts.lock_mode =  userdata.opts.mlock_str ? parse_mlock_mode(userdata.opts.mlock_str) : mlock_mode::NONE;
            userdata.opts.decompress_ratio = userdata.opts.decompress_ratio_str ? folly::to<double>(userdata.opts.decompress_ratio_str) : 0.8;
        }
        catch (runtime_error const& e) {
            std::cerr << "error: " << e.what() << std::endl;
            return 1;
        }
        catch (std::filesystem::filesystem_error const& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }

        if (userdata.opts.decompress_ratio < 0.0 || userdata.opts.decompress_ratio > 1.0) {
            std::cerr << "error: decratio must be between 0.0 and 1.0" << std::endl;
            return 1;
        }

        LOG_PROXY(debug_logger_policy, userdata.lgr);

        LOG_INFO << PRJ_NAME << " version " << PRJ_VERSION_STRING;



        load_filesystem<dwarfs::prod_logger_policy>(userdata, data, size);
    }
    catch (...)
    {
        ret = -1;
    }

    return ret;
}

namespace dwarfs { 

    static const int FUSE_ROOT_ID = 1;

    template <typename LoggerPolicy>
    void load_filesystem(dwarfs_userdata& userdata, const unsigned char data[], const unsigned int size) {
        LOG_PROXY(LoggerPolicy, userdata.lgr);
        auto ti = LOG_TIMED_INFO;
        auto& opts = userdata.opts;

        filesystem_options fsopts;
        fsopts.lock_mode = opts.lock_mode;
        fsopts.block_cache.max_bytes = opts.cachesize;
        fsopts.block_cache.num_workers = opts.workers;
        fsopts.block_cache.decompress_ratio = opts.decompress_ratio;
        fsopts.block_cache.mm_release = !opts.cache_image;
        fsopts.block_cache.init_workers = false;
        fsopts.metadata.enable_nlink = bool(opts.enable_nlink);
        fsopts.metadata.readonly = bool(opts.readonly);

        if (opts.image_offset_str) {
            std::string image_offset{ opts.image_offset_str };

            try 
            {
                fsopts.image_offset = image_offset == "auto"
                    ? filesystem_options::IMAGE_OFFSET_AUTO
                    : folly::to<off_t>(image_offset);
            }
            catch (...) 
            {
                DWARFS_THROW(runtime_error, "failed to parse offset: " + image_offset);
            }
        }

        userdata.fs = filesystem_v2(
            userdata.lgr, std::make_shared<tebako::mfs>(&data, size), fsopts, FUSE_ROOT_ID);

        ti << "file system initialized";
    }

} // namespace dwarfs

