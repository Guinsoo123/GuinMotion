# Overlay triplet for GuinMotion: same as upstream x64-linux with LTO disabled for
# vcpkg-built ports. Mixed GCC installs (e.g. g++-12 objects + gcc-11 lto1) otherwise
# fail at link time with: "bytecode stream ... LTO version 12.0 instead of 11.3".
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

string(APPEND VCPKG_C_FLAGS " -fno-lto")
string(APPEND VCPKG_CXX_FLAGS " -fno-lto")
