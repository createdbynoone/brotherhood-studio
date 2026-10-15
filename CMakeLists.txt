cmake_minimum_required(VERSION 3.21)
project(BrotherhoodStudio VERSION 1.0.9 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Concurrent Multimedia Svg ShaderTools)
qt_standard_project_setup(REQUIRES 6.8)

set(APP_SOURCES
    src/main.cpp
    src/app/studiocontroller.cpp
    src/core/jsonfile.cpp
    src/core/paths.cpp
    src/core/prefs.cpp
    src/core/lock.cpp
    src/platform/mac.mm
    src/platform/memtrim.cpp
    src/prompt/shell.cpp
    src/prompt/prompts.cpp
    src/prompt/memorystore.cpp
    src/prompt/higgsfield.cpp
    src/prompt/claude.cpp
    src/prompt/staging.cpp
    src/prompt/downloader.cpp
    src/prompt/logmodel.cpp
    src/prompt/promptcontroller.cpp
    src/triage/triagetypes.cpp
    src/triage/entryitem.cpp
    src/triage/store.cpp
    src/triage/watcher.cpp
    src/triage/gallerymodel.cpp
    src/triage/thumbprovider.cpp
    src/triage/triagecontroller.cpp
)

qt_add_executable(BrotherhoodStudio MACOSX_BUNDLE ${APP_SOURCES})
set_target_properties(BrotherhoodStudio PROPERTIES
    OUTPUT_NAME "Brotherhood Studio"
    MACOSX_BUNDLE_BUNDLE_NAME "Brotherhood Studio"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.brotherhood.studio.qt"
    MACOSX_BUNDLE_BUNDLE_VERSION "1"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "AppIcon"
    MACOSX_BUNDLE_ICON_NAME "AppIcon"
    MACOSX_BUNDLE_COPYRIGHT "Brotherhood"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.brotherhood.studio.qt")
target_compile_definitions(BrotherhoodStudio PRIVATE APP_VERSION="${PROJECT_VERSION}")

file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS qml/*.qml)
set_source_files_properties(qml/components/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/components/Icons.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/components/Statuses.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(BrotherhoodStudio
    URI Studio
    VERSION 1.0
    QML_FILES ${QML_FILES}
)

# Fonts and images are served from qrc:/assets/... (QML) and :/assets/... (C++)
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/resources/fonts/*.ttf
    ${CMAKE_SOURCE_DIR}/resources/images/*.svg
    ${CMAKE_SOURCE_DIR}/resources/images/*.png)
qt_add_resources(BrotherhoodStudio assets PREFIX "/assets" BASE ${CMAKE_SOURCE_DIR}/resources FILES ${ASSET_FILES})

target_link_libraries(BrotherhoodStudio PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Concurrent Qt6::Multimedia Qt6::Svg
    "-framework AppKit" "-framework Foundation" "-framework CoreGraphics" "-framework ImageIO"
    "-framework AVFoundation" "-framework CoreMedia" "-framework UniformTypeIdentifiers" "-framework IOKit")

# App icon: resources/AppIcon.icon is an Icon Composer document (Liquid Glass icon). actool turns it into
# Assets.car (macOS 26+) plus a flat AppIcon.icns for older systems, straight into the bundle.
add_custom_command(TARGET BrotherhoodStudio POST_BUILD
    COMMAND xcrun actool "${CMAKE_SOURCE_DIR}/resources/AppIcon.icon"
            --compile "$<TARGET_BUNDLE_CONTENT_DIR:BrotherhoodStudio>/Resources"
            --platform macosx --minimum-deployment-target 14.0 --app-icon AppIcon
            --output-partial-info-plist "${CMAKE_BINARY_DIR}/icon-partial.plist" --output-format human-readable-text
    COMMAND sh -c "chmod u+w '$<TARGET_BUNDLE_CONTENT_DIR:BrotherhoodStudio>/Info.plist'; /usr/libexec/PlistBuddy -c 'Add :CFBundleIconName string AppIcon' '$<TARGET_BUNDLE_CONTENT_DIR:BrotherhoodStudio>/Info.plist' 2>/dev/null; touch '$<TARGET_BUNDLE_DIR:BrotherhoodStudio>'"
    COMMENT "Compiling the app icon"
    VERBATIM)
