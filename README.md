##  CI status

[![Ubuntu](https://github.com/tamatebako/libdwarfs/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/ubuntu.yml)   [![MacOS](https://github.com/tamatebako/libdwarfs/actions/workflows/macos.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/macos.yml) [![MacOS-arm64](https://github.com/tamatebako/libdwarfs/actions/workflows/macos-arm64.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/macos-arm64.yml) [![Alpine](https://github.com/tamatebako/libdwarfs/actions/workflows/alpine.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/alpine.yml) [![Windows-MSys](https://github.com/tamatebako/libdwarfs/actions/workflows/windows-msys.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/windows-msys.yml)
[![lint](https://github.com/tamatebako/libdwarfs/actions/workflows/lint.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/lint.yml) [![codecov](https://codecov.io/gh/tamatebako/libdwarfs/branch/main/graph/badge.svg?token=FMMPK27XU7)](https://codecov.io/gh/tamatebako/libdwarfs) [![coverity](https://scan.coverity.com/projects/27408/badge.svg)](https://scan.coverity.com/projects/tamatebako-libdwarfs) [![codeql](https://github.com/tamatebako/libdwarfs/actions/workflows/codeql.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/codeql.yml)

##  libdwarfs wrapper

This is libdwarfs-wr  - a wrapper for https://github.com/mhx/dwarfs core library that provides the following features:
* C interface (as opposed to dwarfs C++ API)
* fd (file descriptor) addressing above dwarfs inode implementation

### CMake project options

* **WITH_TESTS**, default:ON      -- If this option is ON, build script looks for Google test,  installs INCBIN, and build google tests and static test application.
* **WITH_ASAN**, default:ON       -- If this option is ON, address amd memory sanitizer tests are performed.
* **WITH_COVERAGE**, default:ON   -- If this option is ON, test coverage analysis is perfornmed using codecov.
* **RB_W32**, default:OFF         -- If this option is ON, the version integrated with Ruby library is built.
* **WITH_LINK_TEST**, default:ON  -- If this option is ON,  symbolic/hard link tests are enabled.
* **USE_TEMP_FS**, default:OFF    -- If this option is ON, the data for test file system is created under /tmp.  Otherwise in-source location is used.
