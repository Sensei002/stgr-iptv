# ---------------------------------------------------------------------------
# STGR IpTV - source tree
# ---------------------------------------------------------------------------

# Shared configuration header generated from CMake (version, branding).
set(STGR_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/core/version.h.in"
    "${STGR_GENERATED_DIR}/version.h"
    @ONLY
)

# --- core library: core, utils, models (Qt Core only) ----------------------
add_library(stgr_core STATIC
    core/AppPaths.cpp
    core/AppPaths.h
    core/Log.cpp
    core/Log.h
    models/Channel.h
    models/Playlist.cpp
    models/Playlist.h
    utils/StringUtils.cpp
    utils/StringUtils.h
    utils/UrlUtils.cpp
    utils/UrlUtils.h
)
target_include_directories(stgr_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${STGR_GENERATED_DIR}"
)
target_link_libraries(stgr_core PUBLIC Qt6::Core)

# --- playlist library -------------------------------------------------------
add_library(stgr_playlist STATIC
    playlist/M3uParser.cpp
    playlist/M3uParser.h
    playlist/PlaylistManager.cpp
    playlist/PlaylistManager.h
)
target_link_libraries(stgr_playlist PUBLIC
    stgr_core
    Qt6::Core
    Qt6::Network
    Qt6::Concurrent
)

# --- services library -------------------------------------------------------
add_library(stgr_services STATIC
    services/NetworkService.cpp
    services/NetworkService.h
    services/PlaylistFetcher.cpp
    services/PlaylistFetcher.h
    services/LogoCache.cpp
    services/LogoCache.h
    services/SearchIndex.cpp
    services/SearchIndex.h
    services/FavoritesStore.cpp
    services/FavoritesStore.h
    services/HistoryStore.cpp
    services/HistoryStore.h
    services/EpgManager.cpp
    services/EpgManager.h
    services/UpdateService.cpp
    services/UpdateService.h
    services/XmltvParser.cpp
    services/XmltvParser.h
    settings/Settings.cpp
    settings/Settings.h
)
target_link_libraries(stgr_services PUBLIC
    stgr_core
    stgr_playlist
    Qt6::Core
    Qt6::Gui
    Qt6::Network
)

# --- playback library (libVLC when available) -------------------------------
set(STGR_PLAYBACK_SOURCES
    playback/IPlaybackEngine.cpp
    playback/IPlaybackEngine.h
    playback/PlaybackController.cpp
    playback/PlaybackController.h
    playback/EngineFactory.cpp
    playback/EngineFactory.h
)
if(TARGET LibVLC::LibVLC)
    list(APPEND STGR_PLAYBACK_SOURCES playback/VlcPlaybackEngine.cpp playback/VlcPlaybackEngine.h)
    list(APPEND STGR_PLAYBACK_LIBS LibVLC::LibVLC LibVLC::LibVLCCore)
    set(STGR_WITH_VLC_DEFINE "STGR_WITH_VLC=1")
endif()

add_library(stgr_playback STATIC ${STGR_PLAYBACK_SOURCES})
if(STGR_WITH_VLC_DEFINE)
    target_compile_definitions(stgr_playback PRIVATE ${STGR_WITH_VLC_DEFINE})
endif()
target_link_libraries(stgr_playback PUBLIC
    stgr_services
    Qt6::Core
    Qt6::Gui
    ${STGR_PLAYBACK_LIBS}
)

# --- UI library -------------------------------------------------------------
set(STGR_UI_SOURCES
    ui/MainWindow.cpp
    ui/MainWindow.h
    ui/Theme.cpp
    ui/Theme.h
    ui/ChannelListModel.cpp
    ui/ChannelListModel.h
    ui/ChannelDelegate.cpp
    ui/ChannelDelegate.h
    ui/ChannelView.cpp
    ui/ChannelView.h
    ui/ChipButton.cpp
    ui/ChipButton.h
    ui/PlayerPanel.cpp
    ui/PlayerPanel.h
    ui/FlowLayout.cpp
    ui/FlowLayout.h
    ui/MiniPlayerWindow.cpp
    ui/MiniPlayerWindow.h
    ui/pages/HomePage.cpp
    ui/pages/HomePage.h
    ui/pages/LiveTvPage.cpp
    ui/pages/LiveTvPage.h
    ui/pages/SearchPage.cpp
    ui/pages/SearchPage.h
    ui/pages/FilterPage.cpp
    ui/pages/FilterPage.h
    ui/pages/FavoritesPage.cpp
    ui/pages/FavoritesPage.h
    ui/pages/HistoryPage.cpp
    ui/pages/HistoryPage.h
    ui/pages/SettingsPage.cpp
    ui/pages/SettingsPage.h
    ui/pages/AboutPage.cpp
    ui/pages/AboutPage.h
    ui/dialogs/PlaylistDialog.cpp
    ui/dialogs/PlaylistDialog.h
    ui/dialogs/FirstRunDialog.cpp
    ui/dialogs/FirstRunDialog.h
    ui/dialogs/UpdateDialog.cpp
    ui/dialogs/UpdateDialog.h
    resources/resources.qrc
)
add_library(stgr_ui STATIC ${STGR_UI_SOURCES})
target_link_libraries(stgr_ui PUBLIC
    stgr_playback
    stgr_playlist
    Qt6::Widgets
    Qt6::Svg
)

# --- application ------------------------------------------------------------
add_executable(stgr_app WIN32
    main.cpp
)
set_target_properties(stgr_app PROPERTIES
    OUTPUT_NAME "STGR-IpTV"
    WIN32_EXECUTABLE TRUE
)

if(WIN32)
    # Generate the Windows resource file (version info + icon) from the template.
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/app.rc.in" "${STGR_GENERATED_DIR}/app.rc" @ONLY)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/app-icon.ico")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/app-icon.ico"
                       "${STGR_GENERATED_DIR}/app-icon.ico" COPYONLY)
    endif()
    target_sources(stgr_app PRIVATE "${STGR_GENERATED_DIR}/app.rc")
endif()

target_link_libraries(stgr_app PRIVATE
    stgr_ui
    Qt6::Widgets
)

# ---------------------------------------------------------------------------
# Install rules (used by the CI packaging step)
# ---------------------------------------------------------------------------
install(TARGETS stgr_app
    RUNTIME DESTINATION .
)

if(TARGET LibVLC::LibVLC AND LIBVLC_BIN_DIR)
    # Install the VLC runtime next to the executable (libvlc.dll, plugins/, ...).
    install(FILES "${LIBVLC_BIN_DIR}/libvlc.dll" DESTINATION . OPTIONAL)
    install(FILES "${LIBVLC_BIN_DIR}/libvlccore.dll" DESTINATION . OPTIONAL)
    if(LIBVLC_PLUGINS_DIR)
        install(DIRECTORY "${LIBVLC_PLUGINS_DIR}" DESTINATION . OPTIONAL)
    endif()
    if(EXISTS "${LIBVLC_BIN_DIR}/lua")
        install(DIRECTORY "${LIBVLC_BIN_DIR}/lua" DESTINATION . OPTIONAL)
    endif()
endif()
