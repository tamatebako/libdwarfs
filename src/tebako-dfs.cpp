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

#include <tebako-pch.h>
#include <tebako-common.h>
#include <tebako-pch-pp.h>
#include <tebako-dfs.h>
#include <tebako-io-inner.h>
#include <tebako-fd.h>
#include <tebako-dirent.h>
#include <tebako-mfs.h>

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
        userdata->fs.set_num_workers(userdata->opts.workers);

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

    sync_tebako_dstable::dstable.close_all();
    sync_tebako_fdtable::fdtable.close_all();
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

        p->opts.cache_image = 0;
        p->opts.cache_files = 1;


        try {
            p->opts.debuglevel = debuglevel ? logger::parse_level(debuglevel): logger::INFO;

            p->lgr.set_threshold(p->opts.debuglevel);
            p->lgr.set_with_context(p->opts.debuglevel >= logger::DEBUG);

            p->opts.cachesize = cachesize ? dwarfs::parse_size_with_unit(cachesize) : (static_cast<size_t>(512) << 20);
            p->opts.workers = workers ? folly::to<size_t>(workers) : 2;
            p->opts.lock_mode =  mlock ? parse_mlock_mode(mlock) : mlock_mode::NONE;
            p->opts.decompress_ratio = decompress_ratio ? folly::to<double>(decompress_ratio) : 0.8;
        }
        catch (runtime_error const& e) {
            std::cerr << "error: " << e.what() << std::endl;
            return 1;
        }
        catch (std::filesystem::filesystem_error const& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }

        if (p->opts.decompress_ratio < 0.0 || p->opts.decompress_ratio > 1.0) {
            std::cerr << "error: decratio must be between 0.0 and 1.0" << std::endl;
            return 1;
        }

        if (image_offset) {
            std::string img_offset{ image_offset };
            try {
                p->opts.image_offset = (img_offset == "auto") ? filesystem_options::IMAGE_OFFSET_AUTO : folly::to<off_t>(img_offset);
            }
            catch (...)  {
                std::cerr << "error: failed to parse offset: " + img_offset << std::endl;
                return 1;
            }
        }

        LOG_PROXY(debug_logger_policy, p->lgr);
        LOG_INFO << PRJ_NAME << " version " << PRJ_VERSION_STRING;

        (p->opts.debuglevel >= logger::DEBUG) ? load_filesystem<debug_logger_policy>(p) :  load_filesystem<prod_logger_policy>(p);
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
        TEBAKO_SET_LAST_ERROR(err < 0 ? -err : err); // dwarfs returns -ERRNO
    }
    return ret;
}

template <typename Functor, class... Args>
int safe_dwarfs_call(Functor&& fn, uint32_t inode, Args&&... args) {
    //  [TODO]   LOG_PROXY(LoggerPolicy, userdata->lgr);
    //    LOG_DEBUG << __func__;
    int ret = -1;
    auto locked = usd.rlock();
    auto p = *locked;
    if (p) {
        try {
            ret = fn(&p->fs, inode, std::forward<Args>(args)...);
        }
        catch (dwarfs::system_error const& e) {
            TEBAKO_SET_LAST_ERROR(e.get_errno());
        }
        catch (...) {
            TEBAKO_SET_LAST_ERROR(EIO);
        }
    }
    return ret;
}

int dwarfs_access(const char* path, int amode, uid_t uid, gid_t gid) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&, int, uid_t, gid_t)> 
           { [](filesystem_v2* fs, inode_view& inode, int amode, uid_t uid, gid_t gid) -> int { return fs->access(inode, amode, uid, gid); } }, 
           path, amode, uid, gid);
}

int dwarfs_stat(const char* path, struct stat* buf) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&, struct stat*)>
    { [](filesystem_v2* fs, inode_view& inode, struct stat* buf) -> int { return fs->getattr(inode, buf); } },
        path, buf);
}

int dwarfs_readlink(const char* path, std::string& lnk) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&, std::string&)>
    { [](filesystem_v2* fs, inode_view& inode, std::string& lnk) -> int { return fs->readlink(inode, &lnk); } },
        path, lnk);
}

int dwarfs_inode_relative_stat(uint32_t inode, const char* path, struct stat* buf) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, uint32_t, const char*, struct stat*)>
    { [](filesystem_v2* fs, uint32_t inode, const char* path, struct stat* buf) -> int { 
            auto pi = fs->find(inode);
            return pi ? fs->getattr(*pi, buf) : ENOENT;
        } },
        inode, path, buf);
}

int dwarfs_inode_access(uint32_t inode, int amode, uid_t uid, gid_t gid)  noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, uint32_t, int, uid_t, gid_t)>
    { [](filesystem_v2* fs, uint32_t inode, int amode, uid_t uid, gid_t gid) -> int { 
            auto pi = fs->find(inode);
            return pi ? fs->access(*pi, amode, uid, gid) : ENOENT;
        } },
        inode, amode, uid, gid);
}

ssize_t dwarfs_inode_read(uint32_t inode, void* buf, size_t size, off_t offset)  noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, uint32_t, void*, size_t, off_t)>
    { [](filesystem_v2* fs, uint32_t inode, void* buf, size_t size, off_t offset) -> int { return fs->read(inode, (char*)buf, size, offset); } }, 
        inode, buf, size, offset);
}

static int internal_readdir(filesystem_v2* fs, uint32_t inode, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) {
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
                    std::string name(name_view);
                    fs->getattr(entry, &st);
                    cache[cache_size].e.d_ino = st.st_ino;
                    cache[cache_size].e.d_off = cache_start + cache_size;
                    cache[cache_size].e.d_type = DT_UNKNOWN;
                    strncpy(cache[cache_size].e.d_name, name.c_str(), TEBAKO_PATH_LENGTH);
                    cache[cache_size].e.d_name[TEBAKO_PATH_LENGTH] = '\0';
                    cache[cache_size].e.d_reclen = std::max(sizeof(cache[0]), sizeof(cache[0]) + strlen(cache[cache_size].e.d_name) - 256 + 1);
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

int dwarfs_inode_readdir(uint32_t inode, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, uint32_t, tebako_dirent*, off_t, size_t, size_t&, size_t& )> { internal_readdir }, inode, cache, cache_start, buffer_size, cache_size, dir_size);
}

