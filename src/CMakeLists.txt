set(VOXTYPEBRIDGE_SOURCES
    voxtypebridge.cpp
)

set(VOXTYPEBRIDGE_LINK_LIBRARIES
    Fcitx5::Core
    Fcitx5::Config
    Fcitx5::Module::Notifications
    Fcitx5::Module::Clipboard
    Pthread::Pthread
)

add_fcitx5_addon(voxtypebridge ${VOXTYPEBRIDGE_SOURCES})

target_link_libraries(voxtypebridge ${VOXTYPEBRIDGE_LINK_LIBRARIES})

install(TARGETS voxtypebridge DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
configure_file(voxtypebridge.conf.in.in voxtypebridge.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/voxtypebridge.conf.in" voxtypebridge.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/voxtypebridge.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon" COMPONENT config)

