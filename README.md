##  CI status

[![Ubuntu](https://github.com/tamatebako/libdwarfs/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/ubuntu.yml)   [![MacOS](https://github.com/tamatebako/libdwarfs/actions/workflows/macos.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/macos.yml) [![Alpine](https://github.com/tamatebako/libdwarfs/actions/workflows/alpine.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/alpine.yml)
[![Windows-MSys](https://github.com/tamatebako/libdwarfs/actions/workflows/windows-msys.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/windows-msys.yml)

[![Build Status](https://api.cirrus-ci.com/github/tamatebako/libdwarfs.svg?task=ubuntu-aarch64)](https://cirrus-ci.com/github/tamatebako/libdwarfs)

[![lint](https://github.com/tamatebako/libdwarfs/actions/workflows/lint.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/lint.yml) [![codecov](https://codecov.io/gh/tamatebako/libdwarfs/branch/main/graph/badge.svg?token=FMMPK27XU7)](https://codecov.io/gh/tamatebako/libdwarfs) [![coverity](https://scan.coverity.com/projects/27408/badge.svg)](https://scan.coverity.com/projects/tamatebako-libdwarfs) [![codeql](https://github.com/tamatebako/libdwarfs/actions/workflows/codeql.yml/badge.svg)](https://github.com/tamatebako/libdwarfs/actions/workflows/codeql.yml)



##  libdwarfs wrapper

This is libdwarfs-wr  - a wrapper for https://github.com/mhx/dwarfs core library that provides the following features:
* C interface (as opposed to dwarfs C++ API)
* fd (file descriptor) addressing above dwarfs inode implementation


### CMake Project Options

* **WITH_TESTS**, default: ON      -- If this option is ON, the build script looks for Google Test, installs INCBIN, and builds Google tests and a static test application.
* **WITH_ASAN**, default: ON       -- If this option is ON, address and memory sanitizer tests are performed.
* **WITH_COVERAGE**, default: ON   -- If this option is ON, test coverage analysis is performed using Codecov.
* **RB_W32**, default: OFF         -- If this option is ON, the version integrated with the Ruby library is built.
* **WITH_LINK_TEST**, default: ON  -- If this option is ON, symbolic/hard link tests are enabled.

### jemalloc Library Build on macOS

The `libdwarfs` build script creates an additional jemalloc installation on macOS. This is done to satisfy the magic applied by folly during linking but uses a static library.
If the library is created in an emulated environment (QEMU, Rosetta, etc.), there are known issues ([jemalloc issue #1997](https://github.com/jemalloc/jemalloc/issues/1997)) where jemalloc incorrectly defines the number of significant virtual address bits (lg-vaddr parameter).

These issues can be fixed by explicitly setting the `--with-lg-vaddr` parameter for the jemalloc build. We decided not to automate this since we do not feel that we can provide reasonable test coverage. Instead, our build script accepts the `LG_VADDR` environment variable and passes it to the jemalloc build as `--with-lg-vaddr=${LG_VADDR}`.

Simple script to set `LG_VADDR`. Please note that it is provided for illustration only.

```bash
#!/bin/bash

# Check the CPU architecture
ARCH=$(uname -m)

# Check if running under Rosetta 2 emulation
if [[ "$ARCH" == "x86_64" && $(sysctl -n sysctl.proc_translated) == "1" ]]; then
  echo "Running on Apple Silicon under Rosetta 2 emulation"
  export LG_VADDR=39
elif [[ "$ARCH" == "arm64" ]]; then
  echo "Running on Apple Silicon"
  export LG_VADDR=39
else
  echo "Running on Intel Silicon"
  export LG_VADDR=48
fi

echo "Setting lg-vaddr to $LG_VADDR"
```
