project(kcm-about-extended)
set(PROJECT_VERSION "5.5.5")

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set(QT_MIN_VERSION "5.4.0")

find_package(ECM 1.2.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Gui Widgets)
find_package(Wayland)
find_package(epoxy REQUIRED)
set(KF5_MIN_VERSION "5.9.0")
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    KDELibs4Support
    Solid
    NetworkManagerQt
)

# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm-about-distro\")

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
