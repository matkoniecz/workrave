set(SYS_ROOT_PREFIX "")

set(TOOLCHAIN_ROOT "/usr/x86_64-w64-mingw32")
set(SYS_ROOT "${SYS_ROOT_PREFIX}/usr/x86_64-w64-mingw32/sys-root/mingw/")

# If 'bin' is in PATH, zlib1.dll is found. lld cannot directly link with DLLs
# it needs anm import library in 'lib'.
set(ZLIB_ROOT ${SYS_ROOT}/lib)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-clang)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-clang++)
set(CMAKE_RC_COMPILER llvm-rc)

SET (CMAKE_C_FLAGS              "-fuse-ld=lld")
SET (CMAKE_CXX_FLAGS            ${CMAKE_C_FLAGS})

set (WINE wine)
set (ISCC "/workspace/inno/app/ISCC.exe")

set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_ROOT} ${SYS_ROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

include_directories(${TOOLCHAIN_ROOT}/include)
include_directories(${SYS_ROOT}/include)

set(ENV{PKG_CONFIG_LIBDIR} "${SYS_ROOT}/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${SYS_ROOT_PREFIX}")
