# CMake toolchain file for MinGW-w64 cross-compilation (Linux → Windows)
# 用法: cmake -DCMAKE_TOOLCHAIN_FILE=scripts/toolchain-mingw.cmake ...

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# ---- 编译器 ----
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# ---- 搜索路径 ----
set(MINGW_ROOT /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH ${MINGW_ROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ---- Qt6 hints ----
set(Qt6_DIR ${MINGW_ROOT}/lib/cmake/Qt6)
set(Qt6Core_DIR ${MINGW_ROOT}/lib/cmake/Qt6Core)
set(Qt6Gui_DIR ${MINGW_ROOT}/lib/cmake/Qt6Gui)
set(Qt6Widgets_DIR ${MINGW_ROOT}/lib/cmake/Qt6Widgets)
