/**
 *
 * Copyright (c) 2021, [Ribose Inc](https://www.ribose.com).
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
#include "dwarfs/error.h"
#include "dwarfs/filesystem_v2.h"
#include "dwarfs/fstypes.h"
#include "dwarfs/logger.h"
#include "dwarfs/metadata_v2.h"
#include "dwarfs/mmap.h"
#include "dwarfs/options.h"
#include "dwarfs/util.h"


#include "tebako-common.h"
#include "tebako-mfs.h"
#include "tebako-dfs.h"

namespace dwarfs {
    struct options {
        int enable_nlink{ 0 };
        int readonly{ 0 };
        int cache_image{ 0 };
        int cache_files{ 0 };
        size_t cachesize{ 0 };
        size_t workers{ 0 };
        mlock_mode lock_mode{ mlock_mode::NONE };
        double decompress_ratio{ 0.0 };
        logger::level_type debuglevel{ logger::level_type::ERROR };
        off_t image_offset{ 0 };
    };

    struct dwarfs_userdata {
        dwarfs_userdata(std::ostream& os, const void* dt, const unsigned int sz)
            : lgr{ os }, data{ dt }, size{ sz } { }

        const void* data;
        const unsigned int size;

        options opts;
        stream_logger lgr;
        filesystem_v2 fs;
    };

    template <typename LoggerPolicy>
    static void load_filesystem(dwarfs_userdata* userdata) {
        LOG_PROXY(LoggerPolicy, userdata->lgr);
        auto ti = LOG_TIMED_INFO;
        auto& opts = userdata->opts;

        filesystem_options fsopts;
        fsopts.lock_mode = opts.lock_mode;
        fsopts.block_cache.max_bytes = opts.cachesize;
        fsopts.block_cache.num_workers = opts.workers;
        fsopts.block_cache.decompress_ratio = opts.decompress_ratio;
        fsopts.block_cache.mm_release = !opts.cache_image;
        fsopts.block_cache.init_workers = false;
        fsopts.metadata.enable_nlink = bool(opts.enable_nlink);
        fsopts.metadata.readonly = bool(opts.readonly);
        fsopts.image_offset = opts.image_offset;


        userdata->fs = filesystem_v2(
            userdata->lgr, std::make_shared<tebako::mfs>(userdata->data, userdata->size), fsopts);

        ti << "file system initialized";
    }
}



using namespace dwarfs;

// [TODO] thread safe ???
static dwarfs_userdata* usd = NULL;

extern "C" void drop_fs(void) {
    if (usd) {
        delete usd;
    }
    usd = NULL;
}

/*
*   C wrapper for dwarfs load_filesystem implementation  @ tebako-dfs.cpp
*/
extern "C" int load_fs( const void* data, 
                        const unsigned int size,
                        const char* debuglevel,
                        const char* cachesize,
                        const char* workers,
                        const char* mlock,
                        const char* decompress_ratio,
                        const char* image_offset)
{
    try  {
        drop_fs();
        usd = new dwarfs_userdata(std::cerr, data, size);

        usd->opts.cache_image = 0;
        usd->opts.cache_files = 1;


        try {
            usd->opts.debuglevel = debuglevel ? logger::parse_level(debuglevel): logger::INFO;

            usd->lgr.set_threshold(usd->opts.debuglevel);
            usd->lgr.set_with_context(usd->opts.debuglevel >= logger::DEBUG);

            usd->opts.cachesize = cachesize ? dwarfs::parse_size_with_unit(cachesize) : (static_cast<size_t>(512) << 20);
            usd->opts.workers = workers ? folly::to<size_t>(workers) : 2;
            usd->opts.lock_mode =  mlock ? parse_mlock_mode(mlock) : mlock_mode::NONE;
            usd->opts.decompress_ratio = decompress_ratio ? folly::to<double>(decompress_ratio) : 0.8;
        }
        catch (runtime_error const& e) {
            std::cerr << "error: " << e.what() << std::endl;
            return 1;
        }
        catch (std::filesystem::filesystem_error const& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }

        if (usd->opts.decompress_ratio < 0.0 || usd->opts.decompress_ratio > 1.0) {
            std::cerr << "error: decratio must be between 0.0 and 1.0" << std::endl;
            return 1;
        }

        if (image_offset) {
            std::string img_offset{ image_offset };
            try {
                usd->opts.image_offset = (img_offset == "auto") ? filesystem_options::IMAGE_OFFSET_AUTO : folly::to<off_t>(img_offset);
            }
            catch (...)  {
                std::cerr << "error: failed to parse offset: " + img_offset << std::endl;
                return 1;
            }
        }

        LOG_PROXY(debug_logger_policy, usd->lgr);
        LOG_INFO << PRJ_NAME << " version " << PRJ_VERSION_STRING;

        (usd->opts.debuglevel >= logger::DEBUG) ? load_filesystem<debug_logger_policy>(usd) :  load_filesystem<prod_logger_policy>(usd);
    }

    catch (...) {
        return -1;
    }

    return 0;
}

extern "C" int dwarfs_stat(const char* path, struct stat* buf) {
    int err = ENOENT;
    int ret = -1;
    if (usd) {
        try {
            auto inode = usd->fs.find(path + TEBAKO_MOUNT_POINT_LENGTH + 2);
            if (inode &&
                (err = usd->fs.getattr(*inode, buf)) == 0) {
                ret = 0;
            }
        }
        catch (dwarfs::system_error const& e) {
            err = e.get_errno();
        }
        catch (...) {
            err = EIO;
        }
    }
    if (ret) {
        TEBAKO_SET_LAST_ERROR(err);
    }
    return ret;
}

extern "C" int dwarfs_access(const char* path, int amode, uid_t uid, gid_t gid) {
    int err = ENOENT;
    int ret = -1;
    if (usd) {
        try {
            auto inode = usd->fs.find(path + TEBAKO_MOUNT_POINT_LENGTH + 2);
            if ( inode &&
                (err = usd->fs.access(*inode, amode, uid, gid)) == 0) { 
                ret = 0; 
            }
        }    
        catch (dwarfs::system_error const& e) {
            err = e.get_errno();
        }
        catch (...) {
            err = EIO;
        }
    }
    if (ret) {
        TEBAKO_SET_LAST_ERROR(err);
    }
    return ret;
}

