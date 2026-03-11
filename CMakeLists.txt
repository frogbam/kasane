cmake_minimum_required(VERSION 3.22)

project(Kasane VERSION 0.1.0 LANGUAGES CXX)

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

add_custom_target(kasane_frontend_assets ALL
    COMMAND ${CMAKE_COMMAND} -E chdir "${KASANE_FRONTEND_SOURCE_DIR}" cmd /c npm ci
    COMMAND ${CMAKE_COMMAND} -E chdir "${KASANE_FRONTEND_SOURCE_DIR}" cmd /c npm run build
    BYPRODUCTS
        "${KASANE_FRONTEND_DIST_DIR}/index.html"
        "${KASANE_FRONTEND_DIST_DIR}/assets"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Kasane frontend UI"
)

juce_add_gui_app(KasaneApp
    PRODUCT_NAME "Kasane"
    COMPANY_NAME "Kasane Project"
    BUNDLE_ID com.kasane.app
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_sources(KasaneApp
    PRIVATE
        Source/AppState.cpp
        Source/AppState.h
        Source/AudioHostEngine.cpp
        Source/AudioHostEngine.h
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
)

target_compile_definitions(KasaneApp
    PRIVATE
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_PLUGINHOST_LV2=0
        JUCE_PLUGINHOST_VST=0
        KASANE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(KasaneApp
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if (MSVC)
    target_link_options(KasaneApp PRIVATE "/MAP")
endif()

add_dependencies(KasaneApp kasane_frontend_assets)

juce_generate_juce_header(KasaneApp)

add_custom_command(
    TARGET KasaneApp
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:KasaneApp>/frontend"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${KASANE_FRONTEND_DIST_DIR}"
            "$<TARGET_FILE_DIR:KasaneApp>/frontend"
)
