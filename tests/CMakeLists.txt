cmake_minimum_required(VERSION 3.20)
project(display_test_prova_unit_tests CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# doctest pulled at configure time (first build needs internet).
include(FetchContent)
# doctest v2.4.11 has cmake_minimum_required(VERSION 2.x); modern CMake
# rejects it. Force CMake to honor older minimums during FetchContent.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

set(DISPLAY_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(test_model
  test_model.cpp
  test_tag_program.cpp
  ${DISPLAY_ROOT}/TouchGFX/gui/src/model/Model.cpp
  stubs/display_bridge_rx_stub.cpp
)

# Stubs come FIRST so they override the real Core/Inc/display_bridge_rx.h
target_include_directories(test_model PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${DISPLAY_ROOT}/TouchGFX/gui/include
)

target_link_libraries(test_model PRIVATE doctest::doctest)

# Touch-feedback logic is framework-free (header-only), so it links against
# nothing but doctest.
add_executable(test_touch_feedback
  test_touch_feedback.cpp
)
target_include_directories(test_touch_feedback PRIVATE
  ${DISPLAY_ROOT}/TouchGFX/gui/include
)
target_link_libraries(test_touch_feedback PRIVATE doctest::doctest)

add_executable(test_bandy_completion
  test_bandy_completion.cpp
)
target_include_directories(test_bandy_completion PRIVATE
  ${DISPLAY_ROOT}/TouchGFX/gui/include
)
target_link_libraries(test_bandy_completion PRIVATE doctest::doctest)

# Pure (HAL-free) external-flash asset validation logic.
add_executable(test_asset_manifest
  test_asset_manifest.cpp
  ${DISPLAY_ROOT}/Core/Src/asset_manifest.c
)
target_include_directories(test_asset_manifest PRIVATE
  ${DISPLAY_ROOT}/Core/Inc
)
target_link_libraries(test_asset_manifest PRIVATE doctest::doctest)

# Pure (HAL-free) external-flash programming arithmetic (page/sector math).
add_executable(test_asset_flash_layout
  test_asset_flash_layout.cpp
  ${DISPLAY_ROOT}/Core/Src/asset_flash_layout.c
)
target_include_directories(test_asset_flash_layout PRIVATE
  ${DISPLAY_ROOT}/Core/Inc
)
target_link_libraries(test_asset_flash_layout PRIVATE doctest::doctest)

# MSVC: silence "non-standard extension" warnings on uninitialized POD fields
if (MSVC)
  target_compile_options(test_model PRIVATE /W3 /permissive-)
  target_compile_options(test_touch_feedback PRIVATE /W3 /permissive-)
  target_compile_options(test_bandy_completion PRIVATE /W3 /permissive-)
else()
  target_compile_options(test_model PRIVATE -Wall -Wextra)
  target_compile_options(test_touch_feedback PRIVATE -Wall -Wextra)
  target_compile_options(test_bandy_completion PRIVATE -Wall -Wextra)
endif()

enable_testing()
add_test(NAME model_unit_tests COMMAND test_model)
set_tests_properties(model_unit_tests PROPERTIES
  TIMEOUT 30
)
add_test(NAME touch_feedback_unit_tests COMMAND test_touch_feedback)
set_tests_properties(touch_feedback_unit_tests PROPERTIES
  TIMEOUT 30
)
add_test(NAME bandy_completion_unit_tests COMMAND test_bandy_completion)
set_tests_properties(bandy_completion_unit_tests PROPERTIES
  TIMEOUT 30
)
add_test(NAME asset_manifest_unit_tests COMMAND test_asset_manifest)
set_tests_properties(asset_manifest_unit_tests PROPERTIES
  TIMEOUT 30
)
add_test(NAME asset_flash_layout_unit_tests COMMAND test_asset_flash_layout)
set_tests_properties(asset_flash_layout_unit_tests PROPERTIES
  TIMEOUT 30
)
