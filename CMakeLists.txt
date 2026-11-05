cmake_minimum_required(VERSION 3.22)

# Runtime statis di Windows -> tidak perlu install VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

project(DBX266XL VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

file(GLOB DBX_ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.png")
juce_add_binary_data(DbxAssets SOURCES ${DBX_ASSET_FILES})
set_target_properties(DbxAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(DBX266XL
    COMPANY_NAME "LR PRO"
    BUNDLE_ID com.lrpro.dbx266xl
    PRODUCT_NAME "LR 266XL Comp-Gate"
    PLUGIN_MANUFACTURER_CODE LrPr
    PLUGIN_CODE Dx26
    FORMATS VST3 Standalone
    VST3_CATEGORIES Fx Dynamics
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    ICON_BIG   "${CMAKE_CURRENT_SOURCE_DIR}/Icon/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Icon/icon_small.png"
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(DBX266XL PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(DBX266XL PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(DBX266XL
    PRIVATE
        DbxAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# ASIO untuk versi Standalone (Windows). Folder "ASIOSDK" diunduh otomatis oleh workflow GitHub
# (atau letakkan sendiri di sebelah CMakeLists.txt). Jika ada -> ASIO aktif.
if(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ASIOSDK/common/asio.h")
        message(STATUS "ASIO SDK ditemukan -> ASIO aktif di Standalone")
        target_compile_definitions(DBX266XL PUBLIC JUCE_ASIO=1)
        target_include_directories(DBX266XL PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/ASIOSDK/common"
            "${CMAKE_CURRENT_SOURCE_DIR}/ASIOSDK/host"
            "${CMAKE_CURRENT_SOURCE_DIR}/ASIOSDK/host/pc")
    else()
        message(WARNING "ASIO SDK tidak ditemukan -> Standalone dibuat TANPA ASIO")
    endif()
endif()
