# ---------------------------------------------------------------------------
# STGR IpTV - unit tests (Qt Test)
# ---------------------------------------------------------------------------
set(STGR_TEST_COMMON_LIBS
    stgr_playlist
    stgr_services
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Test
)

# Helper: builds and registers a test executable.
function(stgr_add_test name)
    add_executable(${name} "${name}.cpp")
    target_link_libraries(${name} PRIVATE ${STGR_TEST_COMMON_LIBS})
    target_compile_definitions(${name} PRIVATE STGR_UNIT_TEST=1)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

stgr_add_test(tst_m3uparser)
stgr_add_test(tst_playlistmanager)
stgr_add_test(tst_favorites)
stgr_add_test(tst_history)
stgr_add_test(tst_search)
stgr_add_test(tst_settings)
stgr_add_test(tst_urlutils)
