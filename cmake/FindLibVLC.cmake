# FindLibVLC.cmake
#
# Locates the official libVLC SDK for Windows.
#
# The SDK is shipped inside the official VLC Windows archives published by
# VideoLAN, e.g. https://get.videolan.org/vlc/3.0.21/win64/vlc-3.0.21-win64.7z
# An extracted archive root contains:
#
#   libvlc.dll, libvlccore.dll, plugins/, lua/, sdk/include, sdk/lib
#
# Usage:
#   set(VLC_DIR "C:/path/to/vlc-3.0.21")   # or -DVLC_DIR=... or env VLC_DIR
#   find_package(LibVLC)
#
# Imported targets:
#   LibVLC::LibVLC, LibVLC::LibVLCCore
#
# Output variables:
#   LIBVLC_INCLUDE_DIR   - path to the vlc/ headers
#   LIBVLC_SDK_DIR       - path to the sdk/ folder
#   LIBVLC_LIBRARY       - libvlc import library
#   LIBVLCCORE_LIBRARY   - libvlccore import library
#   LIBVLC_BIN_DIR       - directory that holds libvlc.dll / libvlccore.dll
#   LIBVLC_PLUGINS_DIR   - directory holding the VLC plugins/ folder
#   LibVLC_FOUND         - TRUE when everything was located

if(NOT DEFINED VLC_DIR)
    set(VLC_DIR "$ENV{VLC_DIR}")
endif()
set(VLC_DIR "${VLC_DIR}" CACHE STRING "Root of an extracted official VLC Windows archive (contains libvlc.dll, plugins/, sdk/)")

if(VLC_DIR AND NOT EXISTS "${VLC_DIR}")
    message(WARNING "VLC_DIR is set but does not exist: ${VLC_DIR}")
endif()

find_path(LIBVLC_INCLUDE_DIR
    NAMES vlc/vlc.h
    HINTS "${VLC_DIR}/sdk"
    PATH_SUFFIXES include
    NO_DEFAULT_PATH)

find_path(LIBVLC_SDK_DIR
    NAMES vlc/vlc.h
    HINTS "${VLC_DIR}/sdk"
    PATH_SUFFIXES include
    NO_DEFAULT_PATH)
if(LIBVLC_SDK_DIR)
    get_filename_component(LIBVLC_SDK_DIR "${LIBVLC_SDK_DIR}" DIRECTORY)
endif()

find_library(LIBVLC_LIBRARY
    NAMES libvlc
    HINTS "${LIBVLC_SDK_DIR}"
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH)

find_library(LIBVLCCORE_LIBRARY
    NAMES libvlccore
    HINTS "${LIBVLC_SDK_DIR}"
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH)

# Runtime directory: the archive root itself contains libvlc.dll
find_path(LIBVLC_BIN_DIR
    NAMES libvlc.dll
    HINTS "${VLC_DIR}"
    PATH_SUFFIXES bin
    NO_DEFAULT_PATH)

if(LIBVLC_BIN_DIR AND EXISTS "${LIBVLC_BIN_DIR}/plugins")
    set(LIBVLC_PLUGINS_DIR "${LIBVLC_BIN_DIR}/plugins")
else()
    set(LIBVLC_PLUGINS_DIR "" CACHE PATH "VLC plugins directory")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibVLC
    REQUIRED_VARS LIBVLC_INCLUDE_DIR LIBVLC_LIBRARY LIBVLCCORE_LIBRARY LIBVLC_BIN_DIR
    FAIL_MESSAGE "libVLC SDK not found. Set -DVLC_DIR=<extracted VLC archive>.")

if(LibVLC_FOUND AND NOT TARGET LibVLC::LibVLC)
    add_library(LibVLC::LibVLC UNKNOWN IMPORTED)
    set_target_properties(LibVLC::LibVLC PROPERTIES
        IMPORTED_LOCATION "${LIBVLC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBVLC_INCLUDE_DIR}")

    add_library(LibVLC::LibVLCCore UNKNOWN IMPORTED)
    set_target_properties(LibVLC::LibVLCCore PROPERTIES
        IMPORTED_LOCATION "${LIBVLCCORE_LIBRARY}")

    mark_as_advanced(LIBVLC_INCLUDE_DIR LIBVLC_SDK_DIR LIBVLC_LIBRARY
        LIBVLCCORE_LIBRARY LIBVLC_BIN_DIR LIBVLC_PLUGINS_DIR)
endif()
