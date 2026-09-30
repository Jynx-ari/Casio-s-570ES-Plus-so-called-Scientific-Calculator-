# CMake generated Testfile for 
# Source directory: /home/Jay/Downloads/casio_ui_sim
# Build directory: /home/Jay/Downloads/casio_ui_sim/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test("calc_engine_test" "/home/Jay/Downloads/casio_ui_sim/build/calc_engine_test")
set_tests_properties("calc_engine_test" PROPERTIES  _BACKTRACE_TRIPLES "/home/Jay/Downloads/casio_ui_sim/CMakeLists.txt;38;add_test;/home/Jay/Downloads/casio_ui_sim/CMakeLists.txt;0;")
add_test("calc_features_test" "/home/Jay/Downloads/casio_ui_sim/build/calc_features_test")
set_tests_properties("calc_features_test" PROPERTIES  _BACKTRACE_TRIPLES "/home/Jay/Downloads/casio_ui_sim/CMakeLists.txt;43;add_test;/home/Jay/Downloads/casio_ui_sim/CMakeLists.txt;0;")
subdirs("_deps/u8g2-build")
