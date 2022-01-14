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
#include <tebako-common.h>
#include <tebako-pch-pp.h>

namespace fs = std::filesystem;

//  Current working direcory (within tebako memfs)
//  RW lock implemented with folly tooling
//  github.com/facebook/folly/blob/master/folly/docs/Synchronized

class tebako_path_s {
private:
	fs::path p;

public:
//	Gets current working directory
	virtual const char* get_cwd(tebako_path_t cwd) {
		const char* n = p.c_str();
		size_t len = std::min(strlen(n), TEBAKO_PATH_LENGTH);
		memcpy(cwd, n, len);
		cwd[len] = '\0';
		return cwd;
	}

//	Sets current working directory to lexically normal path
	virtual void set_cwd(const char* path) {
		if (path) {
			p.assign(path);
			p += "/";
			p = p.lexically_normal();
		}
		else {
			p.assign("");
		}
	}

//	Checks if the current cwd path is withing tebako memfs
	virtual bool is_in(void) {
		return !p.empty();
	}

//  Expands a path withing tebako memfs
	virtual const char* expand_path(tebako_path_t expanded_path, const char* path) {
		const char* ret = NULL;
		if (path != NULL) {
			fs::path rpath = (p / path).lexically_normal();
			const char* rp = rpath.c_str();
			size_t len = std::min(strlen(rp), TEBAKO_PATH_LENGTH);
			memcpy(expanded_path, rp, len);
			expanded_path[len] = '\0';
			ret = expanded_path;
		}
		return ret;
	}
};

template <typename LoggerPolicy>
class tebako_path_s_l: public tebako_path_s {
private:
    LOG_PROXY_DECL(LoggerPolicy);
public:
    tebako_path_s_l(dwarfs::logger& lgr):
        tebako_path_s(),
        LOG_PROXY_INIT(lgr) {
			LOG_TRACE << __func__ << " [ constructing ] ";
	}

    virtual ~tebako_path_s_l() {
        LOG_TRACE << __func__ << " [ destroying ] ";
	}

    virtual const char* get_cwd(tebako_path_t cwd) {
	    tebako_path_s::get_cwd(cwd);
	    LOG_TRACE << __func__ << " returning [ " << cwd << " ]";
		return cwd;
	}

	virtual void set_cwd(const char* path) {
        LOG_TRACE << __func__ << " setting [ " << (path ? path : "NULL") << " ]";
	    tebako_path_s::set_cwd(path);
	}

	virtual bool is_in(void) {
	    bool ret = tebako_path_s::is_in();
        LOG_TRACE << __func__ << " [ " << (ret ? "true" : "false") << " ]";
        return ret;
	}

    virtual const char* expand_path(tebako_path_t expanded_path, const char* path) {
        tebako_path_s::expand_path(expanded_path, path);
        LOG_TRACE << __func__ << " expanding [ " << (path ? path : NULL) << " --> " << expanded_path << " ]";
        return expanded_path;
    }

};

static folly::Synchronized<tebako_path_s *> tebako_cwd{ NULL };

void tebako_init_cwd(dwarfs::logger& lgr, bool need_debug_policy) {
    auto locked = tebako_cwd.wlock();
    *locked = (need_debug_policy) ? static_cast<tebako_path_s *>(new tebako_path_s_l<dwarfs::debug_logger_policy>(lgr)):
	                                static_cast<tebako_path_s *>(new tebako_path_s_l<dwarfs::prod_logger_policy>(lgr));
}

void tebako_drop_cwd(void) {
    auto locked = tebako_cwd.wlock();
	if (*locked) {
        delete *locked;
	    *locked = NULL;
	}
}

//	Gets current working directory
const char* tebako_get_cwd(tebako_path_t cwd) {
    auto locked = tebako_cwd.rlock();
    return (*locked) ? (*locked)->get_cwd(cwd) : "";
}

//	Sets current working directory to lexically normal path
bool tebako_set_cwd(const char* path) {
	bool ret = false;
	try {
	    auto locked = tebako_cwd.wlock();
		if (*locked) {
	        (*locked)->set_cwd(path);
		    ret = true;
		}
	}
	catch (...) {
	}
	return ret;
}

//  Checks if a path is withing tebako memfs
bool is_tebako_path(const char* path) {
    return (path != NULL &&
            (strncmp((path), "/" TEBAKO_MOUNT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 1) == 0
#ifdef _WIN32
            || strncmp(path, "\\" TEBAKO_MOUNT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 1) == 0
            || strncmp(path + 1, ":/" TEBAKO_MOUNT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 2) == 0
            || strncmp(path + 1, ":\\" TEBAKO_MOUNT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 2) == 0
            || strncmp(path, "//?/" TEBAKO_MOUNT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 3) == 0
            || strncmp(path, "\\\\?\\" TEBAKO_MOUNT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 3) == 0
            || ((strncmp(path, "\\\\?\\", 4) == 0 || strncmp(path, "//?/", 4) == 0) &&
                (strncmp(path + 5, ":/" TEBAKO_MOUNT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 2) == 0 ||
                 strncmp(path + 5, ":\\" TEBAKO_MOUNT_POINT, TEBAKO_MOUNT_POINT_LENGTH + 2) == 0
                )
#endif
            ));
}

//	Checks if the current cwd path is withing tebako memfs
bool is_tebako_cwd(void) {
    auto locked = tebako_cwd.rlock();
    return (*locked) ? (*locked)->is_in() : false;
}

//  Expands a path withing tebako memfs
const char* tebako_expand_path(tebako_path_t expanded_path, const char* path) {
	const char* p = NULL;
	try {
	    auto locked = tebako_cwd.rlock();
		if (*locked) {
	        p = (*locked)->expand_path(expanded_path, path);
		}
	}
	catch (...) {
	}
	return p;
}

//  Returns tebako path is cwd if within tebako memfs
//  NULL otherwise
const char* to_tebako_path(tebako_path_t t_path, const char* path) {
	const char* p_path = NULL;
	try {
	    fs::path p = path;
	    p = p.lexically_normal();
	    auto locked = tebako_cwd.rlock();
	    if ((*locked) && (*locked)->is_in() && p.is_relative()) {
		    p_path = (*locked)->expand_path(t_path, p.c_str());
	    }
		else if (is_tebako_path(path)) {
			const char* pp = p.c_str();
			size_t len = std::min(strlen(pp), TEBAKO_PATH_LENGTH);
			memcpy(t_path, pp, len);
			t_path[len] = '\0';
			p_path = t_path;
		}
	}
	catch (...) {
	}
	return p_path;
}

