cmake_minimum_required(VERSION 3.21)

project(STGR_IpTV
    VERSION 1.0.8
    DESCRIPTION "STGR IpTV - lightweight Windows live TV player by STEiGER Dojo"
    HOMEPAGE_URL "https://github.com/Sensei002/stgr-iptv"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(STGR_REQUIRE_VLC "Fail configuration if the libVLC SDK is not found" OFF)
option(STGR_BUILD_TESTS "Build the unit test suite" ON)
option(STGR_ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# ---------------------------------------------------------------------------
# Qt 6 (Widgets, Network, Svg, Concurrent, Test)
# ---------------------------------------------------------------------------
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network Svg Concurrent Test)

# ---------------------------------------------------------------------------
# libVLC SDK (optional at configure time; required for playback in production)
# ---------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(LibVLC)

if(STGR_REQUIRE_VLC AND NOT LibVLC_FOUND)
    message(FATAL_ERROR
        "The libVLC SDK was not found. Pass -DVLC_DIR=<extracted VLC archive> "
        "(or set the VLC_DIR environment variable) pointing at a directory that "
        "contains libvlc.dll, the plugins/ folder and the sdk/ folder.")
endif()

if(LibVLC_FOUND)
    message(STATUS "libVLC SDK found: include=${LIBVLC_INCLUDE_DIR}")
    message(STATUS "libVLC runtime dir: ${LIBVLC_BIN_DIR}")
else()
    message(WARNING
        "libVLC SDK not found - the application will build without a playback "
        "backend. Set VLC_DIR to enable playback.")
endif()

# ---------------------------------------------------------------------------
# Warning policy (MSVC: /W4 + /WX; Clang/GCC: -Wall -Wextra -Werror)
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4)
    if(STGR_ENABLE_WARNINGS_AS_ERRORS)
        # Generator-expression so /WX applies to the Release config of the
        # multi-config Visual Studio generator used by CI.
        add_compile_options($<$<CONFIG:Release>:/WX>)
    endif()
else()
    add_compile_options(-Wall -Wextra)
    if(STGR_ENABLE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

# ---------------------------------------------------------------------------
# Updater repository (used only for the optional update check; keep in sync
# with the repository you publish releases to).
# ---------------------------------------------------------------------------
set(STGR_UPDATER_REPO "Sensei002/stgr-iptv" CACHE STRING
    "GitHub repository (owner/name) the optional updater checks for releases")

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
add_subdirectory(src)

if(STGR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
