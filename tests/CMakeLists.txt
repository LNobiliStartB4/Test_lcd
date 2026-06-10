cmake_minimum_required(VERSION 3.20)
project(display_test_prova_unit_tests CXX)

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

# MSVC: silence "non-standard extension" warnings on uninitialized POD fields
if (MSVC)
  target_compile_options(test_model PRIVATE /W3 /permissive-)
  target_compile_options(test_touch_feedback PRIVATE /W3 /permissive-)
else()
  target_compile_options(test_model PRIVATE -Wall -Wextra)
  target_compile_options(test_touch_feedback PRIVATE -Wall -Wextra)
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
