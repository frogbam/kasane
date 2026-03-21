cmake_minimum_required(VERSION 3.22)

project(kasane VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
)

FetchContent_MakeAvailable(juce)

list(APPEND CMAKE_MODULE_PATH "${juce_SOURCE_DIR}/extras/Build/CMake")
find_package(WebView2 REQUIRED)

find_program(NPM_EXECUTABLE npm REQUIRED)

set(KASANE_FRONTEND_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/frontend")
set(KASANE_FRONTEND_DIST_DIR "${KASANE_FRONTEND_SOURCE_DIR}/dist")
set(KASANE_FRONTEND_NPM_STAMP "${CMAKE_CURRENT_BINARY_DIR}/kasane_frontend_npm_install.stamp")
set(KASANE_FRONTEND_EMBEDDED_ASSETS
    "${KASANE_FRONTEND_DIST_DIR}/index.html"
    "${KASANE_FRONTEND_DIST_DIR}/assets/app.js"
    "${KASANE_FRONTEND_DIST_DIR}/assets/app.css"
    "${KASANE_FRONTEND_DIST_DIR}/assets/Inter-400.ttf"
    "${KASANE_FRONTEND_DIST_DIR}/assets/Inter-500.ttf"
    "${KASANE_FRONTEND_DIST_DIR}/assets/Inter-600.ttf"
    "${KASANE_FRONTEND_DIST_DIR}/assets/Inter-700.ttf"
    "${KASANE_FRONTEND_DIST_DIR}/assets/JetBrainsMono-400.ttf"
    "${KASANE_FRONTEND_DIST_DIR}/assets/JetBrainsMono-500.ttf"
    "${KASANE_FRONTEND_DIST_DIR}/assets/JetBrainsMono-600.ttf"
    "${KASANE_FRONTEND_DIST_DIR}/assets/JetBrainsMono-700.ttf"
)

file(GLOB_RECURSE KASANE_FRONTEND_BUILD_INPUTS CONFIGURE_DEPENDS
    "${KASANE_FRONTEND_SOURCE_DIR}/index.html"
    "${KASANE_FRONTEND_SOURCE_DIR}/src/*"
    "${KASANE_FRONTEND_SOURCE_DIR}/package.json"
    "${KASANE_FRONTEND_SOURCE_DIR}/package-lock.json"
    "${KASANE_FRONTEND_SOURCE_DIR}/tsconfig.json"
    "${KASANE_FRONTEND_SOURCE_DIR}/vite.config.ts"
)

add_custom_command(
    OUTPUT "${KASANE_FRONTEND_NPM_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E chdir "${KASANE_FRONTEND_SOURCE_DIR}" cmd /c npm ci
    COMMAND ${CMAKE_COMMAND} -E touch "${KASANE_FRONTEND_NPM_STAMP}"
    DEPENDS
        "${KASANE_FRONTEND_SOURCE_DIR}/package.json"
        "${KASANE_FRONTEND_SOURCE_DIR}/package-lock.json"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Installing Kasane frontend dependencies"
)

add_custom_target(kasane_frontend_deps
    DEPENDS "${KASANE_FRONTEND_NPM_STAMP}"
)

add_custom_command(
    OUTPUT ${KASANE_FRONTEND_EMBEDDED_ASSETS}
    COMMAND ${CMAKE_COMMAND} -E chdir "${KASANE_FRONTEND_SOURCE_DIR}" cmd /c npm run build
    DEPENDS
        kasane_frontend_deps
        ${KASANE_FRONTEND_BUILD_INPUTS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Kasane frontend UI"
)

add_custom_target(kasane_frontend_assets ALL
    DEPENDS ${KASANE_FRONTEND_EMBEDDED_ASSETS}
)

juce_add_binary_data(kasane_frontend_data
    NAMESPACE KasaneFrontendData
    HEADER_NAME KasaneFrontendData.h
    SOURCES
        ${KASANE_FRONTEND_EMBEDDED_ASSETS}
)

add_dependencies(kasane_frontend_data kasane_frontend_assets)

juce_add_gui_app(kasane
    PRODUCT_NAME "Kasane"
    COMPANY_NAME "Kasane Project"
    BUNDLE_ID com.kasane.app
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon.svg"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon.svg"
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_sources(kasane
    PRIVATE
        Source/AppState.cpp
        Source/AppState.h
        Source/AudioHostEngine.cpp
        Source/AudioHostEngine.h
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
        Source/PresetStore.cpp
        Source/PresetStore.h
        Source/SplashWindow.cpp
        Source/SplashWindow.h
)

target_compile_definitions(kasane
    PRIVATE
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_ASIO=1
        JUCE_WASAPI=1
        JUCE_DIRECTSOUND=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_PLUGINHOST_LV2=0
        JUCE_PLUGINHOST_VST=0
)

target_link_libraries(kasane
    PRIVATE
        kasane_frontend_data
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if (MSVC)
    target_link_options(kasane PRIVATE "/MAP")
endif()

add_dependencies(kasane kasane_frontend_assets kasane_frontend_data)

juce_generate_juce_header(kasane)
