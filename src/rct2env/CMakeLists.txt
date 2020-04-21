
# CMAKE project for openrct2-ui (UI build of OpenRCT2)
cmake_minimum_required(VERSION 3.9)
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif ()

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Options
option(DISABLE_OPENGL "Disable OpenGL support.")

# Third party libraries
if (MSVC)
    find_package(jansson REQUIRED)
    find_package(SDL2 REQUIRED)
    find_library(SPEEX_LDFLAGS libspeexdsp)
else ()
    PKG_CHECK_MODULES(JANSSON REQUIRED jansson>=2.5)
    PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
    PKG_CHECK_MODULES(SPEEX REQUIRED speexdsp)
endif ()


# Dependencies
## PyTorch
if (NOT TORCH_FOUND)
    find_package(Torch REQUIRED)
    if (TORCH_CXX_FLAGS)
        set(CMAKE_CXX_FLAGS ${TORCH_CXX_FLAGS})
    endif()
endif (NOT TORCH_FOUND)
message(${TORCH_FOUND})
message(${TORCH_CXX_FLAGS})
message(${TORCH_LIBRARIES})
message(${TORCH_INCLUDE_DIRS})
message(${SDL2_INCLUDE_DIRS})
#find_package(Torch REQUIRED)

if (NOT DISABLE_OPENGL)
    # GL doesn't work nicely with macOS, while find_package doesn't work with multiarch on Ubuntu.
    if (APPLE)
        find_package(OpenGL REQUIRED)
    elseif (NOT WIN32)
        PKG_CHECK_MODULES(GL REQUIRED gl)
    endif ()
endif ()

# Sources
# FIXME: smuggle ui stuff. Excluding main loop. Is there a better way?
file(GLOB_RECURSE RCT2ENV_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../openrct2-ui/*.cpp"
   )
list(REMOVE_ITEM RCT2ENV_SOURCES 
    "${CMAKE_CURRENT_LIST_DIR}/../openrct2-ui/Ui.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../openrct2-ui/UiContext.cpp"
    )
file(GLOB_RECURSE RCT2ENV_HEADERS 
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/../*.h"
    "${CMAKE_CURRENT_LIST_DIR}/../*.hpp"
    )
list(REMOVE_ITEM RCT2ENV_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/../openrct2-ui/UiContext.h"
    )
list(REMOVE_ITEM TORCH_INCLUDE_DIRS
    "/home/sme/libtorch/lib/libc10.so"
    )

if (APPLE)
    file(GLOB_RECURSE RCT2ENV_MM_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.mm")
    set_source_files_properties(${RCT2ENV_MM_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++ -fmodules")
endif ()

# Outputs
set (PROJECT rct2env)
project(${PROJECT} CXX)
add_executable(${PROJECT} ${RCT2ENV_SOURCES} ${RCT2ENV_MM_SOURCES})
SET_CHECK_CXX_FLAGS(${PROJECT})
ipo_set_target_properties(${PROJECT})

target_link_libraries(${PROJECT} torch ${TORCH_LIBRARIES})
target_link_libraries(${PROJECT} torch ${TORCH_LIBRARIES})
target_link_libraries(${PROJECT} "libopenrct2"
                                 "${CMAKE_CURRENT_LIST_DIR}/../../../pytorch-cpp-rl/build/libcpprl.a"
                                 ${SDL2_LDFLAGS}
                                 ${SPEEX_LDFLAGS})
target_link_platform_libraries(${PROJECT})

if (NOT DISABLE_OPENGL)
    if (WIN32)
        target_link_libraries(${PROJECT} opengl32)
    elseif (APPLE)
        target_link_libraries(${PROJECT} ${OPENGL_LIBRARY})
    else ()
        target_link_libraries(${PROJECT} ${GL_LIBRARIES})
    endif ()
endif ()

# Includes
target_include_directories(${PROJECT} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/.."
                                              ${SPEEX_INCLUDE_DIRS}
                                              ${TORCH_INCLUDE_DIRS}
                                              "${CMAKE_CURRENT_LIST_DIR}/../openrct2-ui"
                                              "${CMAKE_CURRENT_LIST_DIR}/../../../pytorch-cpp-rl/include"
                                              "${CMAKE_CURRENT_LIST_DIR}/../../../pytorch-cpp-rl/example/lib/spdlog/include"
                                              "${CMAKE_CURRENT_LIST_DIR}/../../../pytorch-cpp-rl/example/lib/spdlog/include"
                                              ${SPEEX_INCLUDE_DIRS})
target_include_directories(${PROJECT} SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})


# Compiler flags
if (WIN32)
    # mingw complains about "%zu" not being a valid format specifier for printf, unless we
    # tell it that it is
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__USE_MINGW_ANSI_STDIO=1")
    target_link_libraries(${PROJECT} comdlg32)
    if (MSVC)
        target_link_libraries(${PROJECT} SDL2::SDL2-static)
        target_include_directories(${PROJECT} SYSTEM PRIVATE SDL2::SDL2-static)
    else ()
        # mingw does not provide proper CMake information like other configurations
        find_path(SDL2_INCLUDE_DIRS SDL2/SDL.h)
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            find_library(SDL2_LDFLAGS sdl2d)
        else ()
            find_library(SDL2_LDFLAGS sdl2)
        endif ()
        # Hardcode some of the libraries used by mingw builds
        target_link_libraries(${PROJECT} imm32 winmm setupapi version)
    endif ()
endif ()
if (MSVC)
    # Add DPI-aware manifest
    # HACK Add /MANIFEST:NO first to prevent cmake from adding its own manifest arguments
    if (NOT $ENV{VCToolsInstallDir} STREQUAL "")
        set(MANIFEST_DPI "$ENV{VCToolsInstallDir}\\Include\\Manifest\\PerMonitorHighDPIAware.manifest")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO /MANIFEST /manifest:embed /manifestinput:\"${MANIFEST_DPI}\"")
    endif ()
endif ()

# Defines
if (DISABLE_OPENGL)
    add_definitions(-DDISABLE_OPENGL)
else ()
    # Makes OpenGL function get queried in run-time rather than linked-in
    add_definitions(-DOPENGL_NO_LINK)
endif ()
if (MSVC)
    add_definitions(-D__DISABLE_DLL_PROXY__)
endif ()

# Add headers check to verify all headers carry their dependencies.
# Only valid for Clang for now:
# - GCC 8 does not support -Wno-pragma-once-outside-header
# - Other compilers status unknown
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(OPENRCT2_HEADERS_CHECK ${RCT2ENV_HEADERS})
    # OpenGLAPIProc.h is not meant to be included directly.
    list(REMOVE_ITEM OPENRCT2_HEADERS_CHECK "${CMAKE_CURRENT_LIST_DIR}/drawing/engines/opengl/OpenGLAPIProc.h")
    add_library(${PROJECT}-headers-check OBJECT ${OPENRCT2_HEADERS_CHECK})
    set_target_properties(${PROJECT}-headers-check PROPERTIES LINKER_LANGUAGE CXX)
    set_source_files_properties(${OPENRCT2_HEADERS_CHECK} PROPERTIES LANGUAGE CXX)
    add_definitions("-x c++ -Wno-pragma-once-outside-header -Wno-unused-const-variable")
    get_target_property(OPENRCT2_INCLUDE_DIRS ${PROJECT} INCLUDE_DIRECTORIES)
    set_target_properties(${PROJECT}-headers-check PROPERTIES INCLUDE_DIRECTORIES "${OPENRCT2_INCLUDE_DIRS}")
endif ()

