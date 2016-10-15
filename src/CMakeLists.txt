configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/Version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DQT_NO_KEYWORDS)

set(kcm_SRCS
    main.cpp
    Module.cpp
    OSRelease.cpp
    openglinfo.cpp
)

ki18n_wrap_ui(kcm_SRCS Module.ui)

add_library(kcm_about_extended MODULE ${kcm_SRCS})

target_link_libraries(kcm_about_extended
    KF5::ConfigWidgets
    KF5::CoreAddons
    KF5::I18n
    KF5::Solid
    KF5::KDELibs4Support
    KF5::NetworkManagerQt
    kwinglutils
    ${epoxy_LIBRARY})

install(TARGETS kcm_about_extended DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES about-extended.desktop DESTINATION ${SERVICES_INSTALL_DIR})
