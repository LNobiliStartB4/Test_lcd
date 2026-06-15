# CMake generated Testfile for 
# Source directory: C:/TouchGFXProjects/Display_test_prova/tests
# Build directory: C:/TouchGFXProjects/Display_test_prova/tests/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[model_unit_tests]=] "C:/TouchGFXProjects/Display_test_prova/tests/build/test_model.exe")
set_tests_properties([=[model_unit_tests]=] PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;87;add_test;C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;0;")
add_test([=[touch_feedback_unit_tests]=] "C:/TouchGFXProjects/Display_test_prova/tests/build/test_touch_feedback.exe")
set_tests_properties([=[touch_feedback_unit_tests]=] PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;91;add_test;C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;0;")
add_test([=[bandy_completion_unit_tests]=] "C:/TouchGFXProjects/Display_test_prova/tests/build/test_bandy_completion.exe")
set_tests_properties([=[bandy_completion_unit_tests]=] PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;95;add_test;C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;0;")
add_test([=[asset_manifest_unit_tests]=] "C:/TouchGFXProjects/Display_test_prova/tests/build/test_asset_manifest.exe")
set_tests_properties([=[asset_manifest_unit_tests]=] PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;99;add_test;C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;0;")
add_test([=[asset_flash_layout_unit_tests]=] "C:/TouchGFXProjects/Display_test_prova/tests/build/test_asset_flash_layout.exe")
set_tests_properties([=[asset_flash_layout_unit_tests]=] PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;103;add_test;C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;0;")
subdirs("_deps/doctest-build")
