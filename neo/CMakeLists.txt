cmake_minimum_required(VERSION 3.16)
project(hellforge LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_DEDICATED "Build dedicated server" OFF)

# Game console (PS5/Xbox/Switch style) porting layer.
# The console service backends in sys/consoles are always compiled so that the
# console code path stays build-verified on every platform. Turning this ON
# additionally defines HF_CONSOLE, which is the switch a real console target
# uses to select its SDK backed implementation of the console services.
option(HF_CONSOLE_SUPPORT "Enable the game console porting layer" OFF)
if(HF_CONSOLE_SUPPORT)
  add_definitions(-DHF_CONSOLE)
endif()

find_package(OpenGL REQUIRED)

#======================================================================
# Platform-specific setup
#======================================================================
if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-DNOMINMAX)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  set(PLATFORM_SOURCES
    sys/win32/*.cpp
  )
else()
  find_package(X11 REQUIRED)
  if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64|arm|arm64|ppc|ppc64|powerpc)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
  endif()
  set(PLATFORM_SOURCES
    sys/linux/*.cpp
  )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /fp:fast")
  else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -fno-omit-frame-pointer -ffast-math")
  endif()
endif()

#======================================================================
# Include directories
#======================================================================
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/hflib
  ${CMAKE_CURRENT_SOURCE_DIR}/framework
  ${CMAKE_CURRENT_SOURCE_DIR}/framework/zlib
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer/OpenGL
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jpeg-6
  ${CMAKE_CURRENT_SOURCE_DIR}/sound
  ${CMAKE_CURRENT_SOURCE_DIR}/swf
  ${CMAKE_CURRENT_SOURCE_DIR}/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/cm
  ${CMAKE_CURRENT_SOURCE_DIR}/aas
  ${CMAKE_CURRENT_SOURCE_DIR}/sys
  ${CMAKE_CURRENT_SOURCE_DIR}/hflib/sys
)

#======================================================================
# Precompiled header
#
# Every translation unit already opens with hflib/hfprecompiled.h, which
# pulls in the whole engine. Turning it into a real precompiled header keeps
# that work from being repeated 350+ times.
#======================================================================
include(CheckCXXSourceCompiles)
# The probe needs the same include paths the targets use, otherwise it fails
# on the transitive "sys/..." includes inside the precompiled header.
set(CMAKE_REQUIRED_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/hflib
  ${CMAKE_CURRENT_SOURCE_DIR}/framework
  ${CMAKE_CURRENT_SOURCE_DIR}/framework/zlib
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer/OpenGL
  ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jpeg-6
  ${CMAKE_CURRENT_SOURCE_DIR}/sound
  ${CMAKE_CURRENT_SOURCE_DIR}/swf
  ${CMAKE_CURRENT_SOURCE_DIR}/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/cm
  ${CMAKE_CURRENT_SOURCE_DIR}/aas
  ${CMAKE_CURRENT_SOURCE_DIR}/sys
  ${CMAKE_CURRENT_SOURCE_DIR}/hflib/sys
)
# Compile only, no link: the precompiled header legitimately references
# engine symbols that are not defined in this probe translation unit.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
check_cxx_source_compiles("
#include \"hflib/hfprecompiled.h\"
int main() { return 0; }
" HAVE_PCH_SUPPORT)

#======================================================================
# hfLib static library
#======================================================================
file(GLOB HFLIB_SOURCES
  hflib/*.cpp
  hflib/audio/*.cpp
  hflib/bv/*.cpp
  hflib/containers/*.cpp
  hflib/geometry/*.cpp
  hflib/hashing/*.cpp
  hflib/math/*.cpp
  hflib/sys/*.cpp
)
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|i386|i686|amd64)")
  list(FILTER HFLIB_SOURCES EXCLUDE REGEX ".*Simd_SSE.*")
endif()
if(WIN32)
  file(GLOB HFLIB_SYS_SOURCES hflib/sys/win32/*.cpp)
else()
  file(GLOB HFLIB_SYS_SOURCES hflib/sys/linux/*.cpp)
endif()
list(APPEND HFLIB_SOURCES ${HFLIB_SYS_SOURCES})
add_library(hflib STATIC ${HFLIB_SOURCES})
target_include_directories(hflib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(HAVE_PCH_SUPPORT)
  target_precompile_headers(hflib PRIVATE hflib/hfprecompiled.h)
endif()

#======================================================================
# Jolt physics (optional)
#
# Vendored under external/jolt (MIT). Off by default so a plain build stays
# exactly as it was: hfPhysicsSystem then runs its built-in impulse solver.
# With HF_ENABLE_JOLT=ON the same contract runs through Jolt instead, and the
# physics system picks Jolt when it is present.
#======================================================================
option(HF_ENABLE_JOLT "Build the vendored Jolt physics backend" OFF)

if(HF_ENABLE_JOLT)
  file(GLOB_RECURSE JOLT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/external/jolt/Jolt/*.cpp)
  add_library(jolt STATIC ${JOLT_SOURCES})
  target_include_directories(jolt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/jolt)
  # These four change JPH_VERSION_ID. hflib/physics/hfJoltConfig.h pins the same
  # values for every translation unit that includes the adapter, so the engine
  # and the library can never disagree about them.
  target_compile_definitions(jolt PUBLIC
    JPH_CROSS_PLATFORM_DETERMINISTIC
    JPH_OBJECT_LAYER_BITS=16
    JPH_ENABLE_ASSERTS=0
    JPH_PROFILE_ENABLED=0
    JPH_DEBUG_RENDERER=0
  )
  target_compile_definitions(hflib PUBLIC HF_PHYSICS_HAS_JOLT=1)
  target_link_libraries(hflib PUBLIC jolt)
endif()

#======================================================================
# Hellforge Engine executable
#======================================================================
file(GLOB ENGINE_SOURCES
  framework/*.cpp
  renderer/*.cpp
  renderer/Color/*.cpp
  renderer/DXT/*.cpp
  renderer/OpenGL/*.cpp
  renderer/jobs/*.cpp
  renderer/jobs/*/*.cpp
  renderer/jpeg-6/*.c
  renderer/jpeg-6/*.cpp
  framework/zlib/*.c
  sound/*.cpp
  sound/Null/*.cpp
  swf/*.cpp
  ui/*.cpp
  cm/*.cpp
  aas/*.cpp
  script/*.cpp
  sys/*.cpp
  sys/consoles/*.cpp
  ${PLATFORM_SOURCES}
)

list(APPEND ENGINE_SOURCES
  renderer/hfUiDrawPass.cpp
)

list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*XAudio2.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*win32.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "framework/hfSession.cpp")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "framework/zlib/.*example.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "framework/zlib/.*test.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "framework/zlib/hfminigzip\\.c")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "renderer/jpeg-6/.*test.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "renderer/jpeg-6/hfjload\\.cpp")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "renderer/jpeg-6/hfjpegtran\\.cpp")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "renderer/jpeg-6/hfjmemansi\\.cpp")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "renderer/jpeg-6/hfjmemdos\\.cpp")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "renderer/jpeg-6/hfjmemname\\.cpp")

add_executable(hellforge ${ENGINE_SOURCES})
target_link_libraries(hellforge hflib)
# NOTE: no target_precompile_headers() here on purpose. This target mixes C
# (framework/zlib, renderer/jpeg-6) and C++ sources, so CMake would try to
# build the precompiled header with the C compiler, which cannot parse
# <typeinfo> and the rest of the C++ engine header. The hflib library above is
# pure C++ and does get the PCH.

# Offline asset pipeline tool (pack/validate/list/extract + .mtr + GLSL).
# Only compile the two tiny hflib files it needs instead of the full library
# (which pulls engine globals like cvarSystem / common / fileSystem).
add_library(hfassetlib STATIC hflib/hfAssetStore.cpp hflib/hfStr.cpp hflib/geometry/hfWinding.cpp)
target_include_directories(hfassetlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(assettool tools/hfAssetTool.cpp tools/hfAssetTool_platform.cpp)
target_include_directories(assettool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(assettool hfassetlib)

if(WIN32)
  target_link_libraries(hellforge
    opengl32
    winmm
    ws2_32
    user32
    gdi32
    shell32
    ole32
  )
else()
  target_link_libraries(hellforge
    ${X11_LIBRARIES}
    ${OPENGL_LIBRARIES}
    z
    dl
    pthread
    m
  )
endif()

set_target_properties(hellforge PROPERTIES
  OUTPUT_NAME hellforge
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Optional CPython bridge: -DHF_PYTHON=ON compiles the real-Python host
# functions into the engine and links libpython (needs python3-dev).
option(HF_PYTHON "Enable the embedded CPython bridge in the script VM" OFF)
if(HF_PYTHON)
  find_package(Python3 COMPONENTS Development REQUIRED)
  target_compile_definitions(hellforge PRIVATE HF_PYTHON)
  target_include_directories(hellforge PRIVATE ${Python3_INCLUDE_DIRS})
  target_link_libraries(hellforge PRIVATE ${Python3_LIBRARIES})
endif()
