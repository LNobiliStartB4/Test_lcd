# CMake generated Testfile for 
# Source directory: C:/TouchGFXProjects/Display_test_prova/tests
# Build directory: C:/TouchGFXProjects/Display_test_prova/tests/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[model_unit_tests]=] "C:/TouchGFXProjects/Display_test_prova/tests/build/test_model.exe")
set_tests_properties([=[model_unit_tests]=] PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;45;add_test;C:/TouchGFXProjects/Display_test_prova/tests/CMakeLists.txt;0;")
subdirs("_deps/doctest-build")
