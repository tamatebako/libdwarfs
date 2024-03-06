#include <stdio.h>

#include <tebako/tebako-defines.h>
#include "tebako-fs.h"


int main(int arc, char *argv[]) {
  printf("Hello, World!\n");

  int ret = load_fs(&gfsData[0], gfsSize, "warn",
                    NULL /* cachesize*/, NULL /* workers */, NULL /* mlock */,
                    NULL /* decompress_ratio*/, NULL /* image_offset */
  );

  return 0;
}

 // $env:Path += ";C:\msys64\ucrt64\bin"
 // cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Release -DPREFER_SYSTEM_GTEST=Off -G Ninja .