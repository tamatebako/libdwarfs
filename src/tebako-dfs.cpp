/**
 *
 * Copyright (c) 2021-2022, [Ribose Inc](https://www.ribose.com).
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
#include <tebako-pch-pp.h>
#include <tebako-common.h>
#include <tebako-dirent.h>
#include <tebako-dfs.h>
#include <tebako-io-rb-w32.h>
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-fd.h>
#include <tebako-mfs.h>

namespace stdfs = std::filesystem;

namespace dwarfs {

    template <typename LoggerPolicy>
    static void load_filesystem(dwarfs_userdata* userdata) {

        std::set_terminate([](){ std::cout << "Unhandled exception" << std::endl; std::abort(); });

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
    tebako_drop_cwd();
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
    std::set_terminate([](){ std::cout << "Unhandled exception" << std::endl; std::abort(); });

    try  {
        drop_fs();

        auto locked = usd.wlock();
        *locked = new dwarfs_userdata(std::cerr, data, size);
        auto p = *locked;

        p->opts.cache_image = 0;
        p->opts.cache_files = 1;


        p->opts.debuglevel = debuglevel ? logger::parse_level(debuglevel): logger::INFO;

        p->lgr.set_threshold(p->opts.debuglevel);
        p->lgr.set_with_context(p->opts.debuglevel >= logger::DEBUG);

        p->opts.cachesize = cachesize ? dwarfs::parse_size_with_unit(cachesize) : (static_cast<size_t>(512) << 20);
        p->opts.workers = workers ? folly::to<size_t>(workers) : 2;
        p->opts.lock_mode =  mlock ? parse_mlock_mode(mlock) : mlock_mode::NONE;
        p->opts.decompress_ratio = decompress_ratio ? folly::to<double>(decompress_ratio) : 0.8;

        if (p->opts.decompress_ratio < 0.0 || p->opts.decompress_ratio > 1.0) {
            std::cerr << "error: decratio must be between 0.0 and 1.0" << std::endl;
            return 1;
        }

        if (image_offset) {
            std::string img_offset{ image_offset };
            if (img_offset == "auto") {
                p->opts.image_offset = filesystem_options::IMAGE_OFFSET_AUTO;
            }
            else {
                auto o1 = folly::tryTo<off_t>(img_offset);
               if (o1.hasValue()) {
                   p->opts.image_offset = o1.value();
               }
               else {
                std::cerr << "error: failed to parse offset: " + img_offset << std::endl;
                return 1;
               }
            }
        }

        LOG_PROXY(debug_logger_policy, p->lgr);
        LOG_INFO << PRJ_NAME << " version " << PRJ_VERSION_STRING;

        tebako_init_cwd(p->lgr, p->opts.debuglevel >= logger::DEBUG);
        (p->opts.debuglevel >= logger::DEBUG) ? load_filesystem<debug_logger_policy>(p) :  load_filesystem<prod_logger_policy>(p);

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


template <typename Functor, class... Args>
int safe_dwarfs_call(Functor&& fn, const char* caller, const char* path, Args&&... args) {
    int err = ENOENT;
    int ret = DWARFS_IO_ERROR;
    auto locked = usd.rlock();
    auto p = *locked;
    if (p) {
        if (p->opts.debuglevel >= logger::DEBUG) {
            LOG_PROXY(debug_logger_policy, p->lgr);
            LOG_DEBUG << caller << " [ " << path << " ]";
        }
        try {
// Normally we remove '/__tebako_memfs__/'
// However, there is also a case when it is memfs root and path isn just '/__tebako_memfs__'
            const char* adj = path[TEBAKO_MOUNT_POINT_LENGTH + 1] == '\0' ?
                path + TEBAKO_MOUNT_POINT_LENGTH + 1 :
                path + TEBAKO_MOUNT_POINT_LENGTH + 2;
            auto inode = p->fs.find(adj);
            if (p->opts.debuglevel >= logger::DEBUG) {
              LOG_PROXY(debug_logger_policy, p->lgr);
              if (inode)
                LOG_DEBUG << "inode: " << inode->inode_num();
              else
                LOG_DEBUG << "inode: not found";
            }
            if (inode) {
                err = fn(&p->fs, *inode, std::forward<Args>(args)...);
                if (err == 0) {
                    ret = DWARFS_IO_CONTINUE;
                }
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
        TEBAKO_SET_LAST_ERROR(err < 0 ? -err : err); // dwarfs returns -ERRNO
    }
    return ret;
}

template <typename Functor, class... Args>
int safe_dwarfs_call(Functor&& fn, const char* caller, uint32_t inode, Args&&... args) {
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
                err = -ret;
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
        TEBAKO_SET_LAST_ERROR(err < 0 ? -err : err); // dwarfs returns -ERRNO
    }
    return ret;
}

int dwarfs_access(const char* path, int amode, uid_t uid, gid_t gid, std::string& lnk) noexcept {
    struct stat st;
    int ret = dwarfs_stat(path, &st, lnk);
    if (ret == DWARFS_IO_CONTINUE) {
        ret = dwarfs_inode_access(st.st_ino, amode, uid, gid);
    }
    return ret;
}

#if defined(TEBAKO_HAS_LSTAT) || defined(RB_W32)
int dwarfs_lstat(const char* path, struct stat* buf) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&, struct stat*)>
    { [](filesystem_v2* fs, inode_view& inode, struct stat* buf) -> int { return fs->getattr(inode, buf); } },
        __func__, path, buf);
}
#endif

int dwarfs_stat(const char* path, struct stat* buf, std::string& lnk) noexcept {
    int ret = safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&, struct stat*)>
              { [](filesystem_v2* fs, inode_view& inode, struct stat* buf) -> int { return fs->getattr(inode, buf); } },
                  __func__, path, buf);
    try {
        if (ret == DWARFS_IO_CONTINUE && S_ISLNK(buf->st_mode)) {
            ret = dwarfs_readlink(path, lnk);
            if (ret == DWARFS_IO_CONTINUE) {
                stdfs::path p_new = path;
                stdfs::path p_lnk = lnk;
                if (p_lnk.is_relative())    p_new.replace_filename(lnk);
                else                        p_new = lnk;
                p_new = p_new.lexically_normal();
                if (is_tebako_path(p_new.generic_string().c_str()))  {
#ifdef RB_W32
                    struct STAT_TYPE _buf;
                    ret = tebako_stat(p_new.generic_string().c_str(), &_buf);
                    buf << _buf;
#else
                    ret = tebako_stat(p_new.generic_string().c_str(), buf);
#endif
                }
                else  ret = DWARFS_S_LINK_OUTSIDE;
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

int dwarfs_readlink(const char* path, std::string& lnk) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, inode_view&, std::string&)>
    { [](filesystem_v2* fs, inode_view& inode, std::string& lnk) -> int { return fs->readlink(inode, &lnk); } },
        __func__, path, lnk);
}

int dwarfs_inode_readlink(uint32_t inode, std::string& lnk) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, uint32_t, std::string&)>
    { [](filesystem_v2* fs, uint32_t inode, std::string& lnk) -> int {
            auto pi = fs->find(inode);
            return pi ? fs->readlink(*pi, &lnk) : DWARFS_IO_ERROR;
        }
    },
    __func__, inode, lnk);
}

static int internal_getattr_relative(uint32_t inode, stdfs::path p_path, struct stat* buf) {
    int ret = DWARFS_IO_ERROR;
    auto locked = usd.rlock();
    auto p = *locked;
    if (p) {
        auto pi = p->fs.find(inode);
        for (auto p_it = p_path.begin(); p_it != p_path.end(); ++p_it) {
            if (!pi) break;
            pi = p->fs.find(pi->inode_num(), p_it->string().c_str());
        }
        ret =  pi ? p->fs.getattr(*pi, buf) : ENOENT;
    }
    return ret;
}

int dwarfs_inode_relative_stat(uint32_t inode, const char* path, struct stat* buf, bool follow) noexcept {
    int ret = safe_dwarfs_call(
        std::function<int(filesystem_v2*, uint32_t, const char*, struct stat*)> {
            [] (filesystem_v2* fs, uint32_t inode, const char* path, struct stat* buf) -> int {
                auto pi = fs->find(inode, path);
                return pi ? fs->getattr(*pi, buf) : ENOENT;
            }
        },
        __func__, inode, path, buf);
        try {
                if (ret == DWARFS_IO_CONTINUE && S_ISLNK(buf->st_mode) && follow) {
                    std::string lnk;
                    ret = dwarfs_inode_readlink(buf->st_ino, lnk);
                    if (ret == DWARFS_IO_CONTINUE) {
                        stdfs::path p_path(lnk);
                        if (p_path.is_relative()) ret =  internal_getattr_relative(inode, p_path, buf);
                        else {
#ifdef RB_W32
                                struct STAT_TYPE _buf;
                                ret =  tebako_stat(p_path.generic_string().c_str(), &_buf);
                                buf << _buf;
#else
                                ret =  tebako_stat(p_path.generic_string().c_str(), buf);
#endif
                        }
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

int dwarfs_inode_access(uint32_t inode, int amode, uid_t uid, gid_t gid)  noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, uint32_t, int, uid_t, gid_t)>
    { [](filesystem_v2* fs, uint32_t inode, int amode, uid_t uid, gid_t gid) -> int {
            auto pi = fs->find(inode);
            return pi ? fs->access(*pi, amode, uid, gid) : ENOENT;
        } },
        __func__, inode, amode, uid, gid);
}

ssize_t dwarfs_inode_read(uint32_t inode, void* buf, size_t size, off_t offset)  noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, uint32_t, void*, size_t, off_t)>
    { [](filesystem_v2* fs, uint32_t inode, void* buf, size_t size, off_t offset) -> int { return fs->read(inode, (char*)buf, size, offset); } },
        __func__, inode, buf, size, offset);
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
#ifndef _WIN32
                    cache[cache_size].e.d_ino = st.st_ino;
#   if __MACH__
                    cache[cache_size].e.d_seekoff = cache_start + cache_size;
#   else
                    cache[cache_size].e.d_off = cache_start + cache_size;
#   endif
                    cache[cache_size].e.d_type = IFTODT(st.st_mode);
                    strncpy(cache[cache_size]._e.d_name, name.c_str(), TEBAKO_PATH_LENGTH);
                    cache[cache_size]._e.d_name[TEBAKO_PATH_LENGTH] = '\0';
                    cache[cache_size].e.d_reclen = sizeof(cache[0]);
#else
#   ifdef RB_W32
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
                    cache[cache_size].e.d_namlen = TEBAKO_PATH_LENGTH-1;
#   else
                    cache[cache_size].e.d_reclen = 0;
                    cache[cache_size].e.d_namlen = std::min(name.length(), sizeof(cache[cache_size].e.d_name)-1);
#   endif
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

int dwarfs_inode_readdir(uint32_t inode, tebako_dirent* cache, off_t cache_start, size_t buffer_size, size_t& cache_size, size_t& dir_size) noexcept {
    return safe_dwarfs_call(std::function<int(filesystem_v2*, uint32_t, tebako_dirent*, off_t, size_t, size_t&, size_t& )> { internal_readdir },
                            __func__, inode, cache, cache_start, buffer_size, cache_size, dir_size);
}
