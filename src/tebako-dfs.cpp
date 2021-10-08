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

#include "tebako-common.h"
#include "tebako-dfs.h"
#include "tebako-io-inner.h"
#include "tebako-mfs.h"

namespace dwarfs {
 
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

// dwarFS user data including fs pointer
// RW lock implemented using folly tooling
// github.com/facebook/folly/blob/master/folly/docs/Synchronized

static folly::Synchronized<dwarfs_userdata*> usd{ NULL };

// Drop previously loaded dwarFS image
extern "C" void drop_fs(void) {
    auto locked = usd.wlock();
    if (*locked) {
        delete *locked;
    }
    *locked = NULL;
}


// Loads dwarFS image
// ["C" wrapper for load_filesystem]
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

        auto locked = usd.wlock();
        *locked = new dwarfs_userdata(std::cerr, data, size);
        auto p = *locked;
        auto opts = p->opts;

        opts.cache_image = 0;
        opts.cache_files = 1;


        try {
            p->opts.debuglevel = debuglevel ? logger::parse_level(debuglevel): logger::INFO;

            p->lgr.set_threshold(opts.debuglevel);
            p->lgr.set_with_context(opts.debuglevel >= logger::DEBUG);

            opts.cachesize = cachesize ? dwarfs::parse_size_with_unit(cachesize) : (static_cast<size_t>(512) << 20);
            opts.workers = workers ? folly::to<size_t>(workers) : 2;
            opts.lock_mode =  mlock ? parse_mlock_mode(mlock) : mlock_mode::NONE;
            opts.decompress_ratio = decompress_ratio ? folly::to<double>(decompress_ratio) : 0.8;
        }
        catch (runtime_error const& e) {
            std::cerr << "error: " << e.what() << std::endl;
            return 1;
        }
        catch (std::filesystem::filesystem_error const& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }

        if (opts.decompress_ratio < 0.0 || opts.decompress_ratio > 1.0) {
            std::cerr << "error: decratio must be between 0.0 and 1.0" << std::endl;
            return 1;
        }

        if (image_offset) {
            std::string img_offset{ image_offset };
            try {
                opts.image_offset = (img_offset == "auto") ? filesystem_options::IMAGE_OFFSET_AUTO : folly::to<off_t>(img_offset);
            }
            catch (...)  {
                std::cerr << "error: failed to parse offset: " + img_offset << std::endl;
                return 1;
            }
        }

        LOG_PROXY(debug_logger_policy, p->lgr);
        LOG_INFO << PRJ_NAME << " version " << PRJ_VERSION_STRING;

        (opts.debuglevel >= logger::DEBUG) ? load_filesystem<debug_logger_policy>(p) :  load_filesystem<prod_logger_policy>(p);
    }

    catch (...) {
        return -1;
    }

    return 0;
}


template <typename Functor, class... Args>
int safe_dwarfs_call(Functor&& fn, const char* path, Args&&... args) {
//  [TODO]   LOG_PROXY(LoggerPolicy, userdata->lgr);
//    LOG_DEBUG << __func__;
    int err = ENOENT;
    int ret = -1;
    auto locked = usd.rlock();
    auto p = *locked;
    if (p) {
        try {
            auto inode = p->fs.find(path + TEBAKO_MOUNT_POINT_LENGTH + 2);
            if (inode) {
                err = fn(&p->fs, *inode, std::forward<Args>(args)...);
                if (err == 0) {
                    ret = 0;
                }
            }
        }
        catch (dwarfs::system_error const& e) {
            err = e.get_errno();
        }
        catch (...) {
            err = EIO;
        }
    }
    if (ret < 0) {
        TEBAKO_SET_LAST_ERROR(err);
    }
    return ret;
}

extern "C" int dwarfs_access(const char* path, int amode, uid_t uid, gid_t gid) {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&, int, uid_t, gid_t)> 
           { [](filesystem_v2* fs, inode_view inode, int amode, uid_t uid, gid_t gid) -> int { return fs->access(inode, amode, uid, gid); } }, 
           path, amode, uid, gid);
}

extern "C" int dwarfs_stat(const char* path, struct stat* buf) {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&, struct stat*)>
           { [](filesystem_v2* fs, inode_view inode, struct stat* buf) -> int { return fs->getattr(inode, buf); } },
           path, buf);
}

extern "C" int dwarfs_find(const char* path) {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&)>
    { [](filesystem_v2*, inode_view inode) -> int { return inode.inode_num(); } },
        path);
}


