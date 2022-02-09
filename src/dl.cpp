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
#include <tebako-io.h>
#include <tebako-io-inner.h>
#include <tebako-fd.h>

using namespace std;
namespace fs = std::filesystem;

typedef map<string, string> tebako_dltable;

class sync_tebako_dltable : public folly::Synchronized<tebako_dltable*> {
private:
	fs::path dl_tmpdir;

    void create_temporary_directory(void) {
		const uint64_t MAX_TRIES = 1024;
	    auto tmp_dir = fs::temp_directory_path();
        fs::path _dl_tmpdir;
        uint64_t i = 1;
        std::random_device dev;
        std::mt19937 prng(dev());
        std::uniform_int_distribution<uint64_t> rand(0);
        while (true) {
            std::stringstream ss;
            ss << std::hex << rand(prng);
            _dl_tmpdir = tmp_dir / ss.str();
            if (fs::create_directory(_dl_tmpdir)) {
				dl_tmpdir = _dl_tmpdir;
                break;
            }
            if (i == MAX_TRIES) {
                throw std::runtime_error("Could not create temporary directory");
            }
            i++;
        }
    }

	void map_name(const char* path, std::string& mapped) {
        const char* adj = path[TEBAKO_MOUNT_POINT_LENGTH + 1] == '\0' ?
                path + TEBAKO_MOUNT_POINT_LENGTH + 1 :
                path + TEBAKO_MOUNT_POINT_LENGTH + 2;
        fs::path _mapped = dl_tmpdir / adj;
		fs::create_directories(_mapped.parent_path());
	    mapped = _mapped;
	}

public:
	sync_tebako_dltable(void) : folly::Synchronized<tebako_dltable*>(new tebako_dltable) {
		create_temporary_directory();
	}
	~sync_tebako_dltable(void) {
		auto p_dltable = *wlock();
		for (auto it = p_dltable->begin(); it != p_dltable->end(); ++it) {
			::unlink(it->second.c_str());
		}
		p_dltable->clear();
		if (!dl_tmpdir.empty()) {
			std::error_code ec;
			fs::remove_all(dl_tmpdir, ec);
		}
	}

	string map2file(const char* path) {
		string ret ="";
		auto p_dltable = *wlock();
		auto p_dl = p_dltable->find(path);
		if (p_dl != p_dltable->end()) {
			ret = p_dl->second;
		}
		else {
			try {
			    std::string mapped;
			    map_name(path, mapped);
				int fh_in = tebako_open(2, path, O_RDONLY);
				size_t f_size = 0;
				if (fh_in < 0) {
					TEBAKO_SET_LAST_ERROR(ENOENT);
					fh_in = -1;
				}
				else {
					struct stat st;
					if (tebako_fstat(fh_in, &st) == -1) {
						TEBAKO_SET_LAST_ERROR(EIO);
					}
					else {
						int fh_out = ::open(mapped.c_str(), O_WRONLY | O_CREAT, st.st_mode);
						if (fh_out == -1) {
							TEBAKO_SET_LAST_ERROR(EIO);
						}
						else {
							f_size = st.st_size;
							const int bsize = 16 * 1024;
							char buf[bsize];
							ssize_t r_size = 1;
							while (f_size > 0 && r_size > 0) {
								ssize_t r_size = tebako_read(fh_in, buf, bsize);
								f_size -= r_size;
								if (r_size != ::write(fh_out, buf, r_size)) {
									r_size = -1;
								}
							}
						}
						if (f_size == 0) {
							ret = mapped;
							(*p_dltable)[path] = mapped;
						}
						else {
							::unlink(mapped.c_str());
							TEBAKO_SET_LAST_ERROR(EIO);
						}
						::close(fh_out);
					}
					tebako_close(fh_in);
				}
			}
			catch(...) {
				TEBAKO_SET_LAST_ERROR(ENOMEM);
			}
		}
		return ret;
	}
	static sync_tebako_dltable dltable;
};

sync_tebako_dltable sync_tebako_dltable::dltable;

extern "C"	void* tebako_dlopen(const char* path, int flags) {
	void* ret = NULL;
//  ...
//	If filename is NULL, then the returned handle is for the main program.
//  ...
	if (path == NULL) {
		ret = ::dlopen(path, flags);
	}
	else {
		tebako_path_t t_path;
		const char* p_path = to_tebako_path(t_path, path);
		if (!p_path) {
			ret = ::dlopen(path, flags);
		}
		else {
			string mapped_path = sync_tebako_dltable::dltable.map2file(p_path);
			if (!mapped_path.empty()) {
				ret = ::dlopen(mapped_path.c_str(), flags);
			}
		}
	}
	return ret;
}
