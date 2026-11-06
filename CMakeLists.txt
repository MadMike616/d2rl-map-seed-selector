cmake_minimum_required(VERSION 3.29)

project(MapSeedSelector VERSION 0.1.36 LANGUAGES CXX RC ASM_MASM)

include(FetchContent)

set(D2RLPLUGIN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(D2RLPLUGIN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    D2RLoaderPluginSDK
    GIT_REPOSITORY https://github.com/D2RLoader/PluginSDK.git
    GIT_TAG 03a5af9e0feb3f95f95e148e330e01f6635950df
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(D2RLoaderPluginSDK)

set(MAP_SEED_SELECTOR_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${MAP_SEED_SELECTOR_GENERATED_DIR}")
set(MAP_SEED_SELECTOR_BUTTON_SPRITE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/add-seed-button.sprite")
set(MAP_SEED_SELECTOR_BUTTON_LOWEND_SPRITE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/add-seed-button.lowend.sprite")
set(MAP_SEED_SELECTOR_MODAL_BUTTON_SPRITE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/modal-button.sprite")
set(MAP_SEED_SELECTOR_MODAL_BUTTON_LOWEND_SPRITE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/modal-button.lowend.sprite")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${MAP_SEED_SELECTOR_BUTTON_SPRITE}"
    "${MAP_SEED_SELECTOR_BUTTON_LOWEND_SPRITE}"
    "${MAP_SEED_SELECTOR_MODAL_BUTTON_SPRITE}"
    "${MAP_SEED_SELECTOR_MODAL_BUTTON_LOWEND_SPRITE}")
set(MAP_SEED_SELECTOR_BUTTON_HEADER
    "${MAP_SEED_SELECTOR_GENERATED_DIR}/add_seed_button_sprite.hpp")
file(WRITE "${MAP_SEED_SELECTOR_BUTTON_HEADER}"
    "#pragma once\n#include <array>\n#include <cstdint>\nnamespace MapSeedSelector::EmbeddedAssets {\n")
foreach(_asset IN ITEMS
        "AddSeedButtonSprite|${MAP_SEED_SELECTOR_BUTTON_SPRITE}"
        "AddSeedButtonLowEndSprite|${MAP_SEED_SELECTOR_BUTTON_LOWEND_SPRITE}"
        "ModalButtonSprite|${MAP_SEED_SELECTOR_MODAL_BUTTON_SPRITE}"
        "ModalButtonLowEndSprite|${MAP_SEED_SELECTOR_MODAL_BUTTON_LOWEND_SPRITE}")
    string(REPLACE "|" ";" _asset_fields "${_asset}")
    list(GET _asset_fields 0 _asset_name)
    list(GET _asset_fields 1 _asset_path)
    file(READ "${_asset_path}" _asset_hex HEX)
    string(LENGTH "${_asset_hex}" _asset_hex_length)
    math(EXPR _asset_byte_count "${_asset_hex_length} / 2")
    string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1,"
        _asset_bytes "${_asset_hex}")
    file(APPEND "${MAP_SEED_SELECTOR_BUTTON_HEADER}"
        "inline constexpr std::array<std::uint8_t, ${_asset_byte_count}> ${_asset_name}{${_asset_bytes}};\n")
endforeach()
file(APPEND "${MAP_SEED_SELECTOR_BUTTON_HEADER}" "}\n")

add_library(map_seed_selector SHARED
    src/plugin.cpp
    src/map_seed_selector.cpp
    src/native_dropdown_art.cpp
    src/map_seed_selector.asm
    src/plugin.rc
)
target_link_libraries(map_seed_selector PRIVATE
    D2RLPlugin::D2RLPlugin
    User32
)
target_compile_features(map_seed_selector PRIVATE cxx_std_20)
target_compile_definitions(map_seed_selector PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
)
target_include_directories(map_seed_selector PRIVATE
    src
    "${MAP_SEED_SELECTOR_GENERATED_DIR}"
)
set_target_properties(map_seed_selector PROPERTIES
    OUTPUT_NAME "d2rl-map-seed-selector"
    PREFIX ""
)

d2rlplugin_embed_config(
    TARGET map_seed_selector
    FILE "${CMAKE_CURRENT_SOURCE_DIR}/config/map-seed-selector.toml"
)

if(MSVC)
    target_compile_options(map_seed_selector PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
    )
    target_link_options(map_seed_selector PRIVATE /INCREMENTAL:NO /Brepro)
endif()
