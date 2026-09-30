# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-src")
  file(MAKE_DIRECTORY "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-src")
endif()
file(MAKE_DIRECTORY
  "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-build"
  "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-subbuild/u8g2-populate-prefix"
  "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-subbuild/u8g2-populate-prefix/tmp"
  "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-subbuild/u8g2-populate-prefix/src/u8g2-populate-stamp"
  "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-subbuild/u8g2-populate-prefix/src"
  "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-subbuild/u8g2-populate-prefix/src/u8g2-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-subbuild/u8g2-populate-prefix/src/u8g2-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/Jay/Downloads/casio_ui_sim/build/_deps/u8g2-subbuild/u8g2-populate-prefix/src/u8g2-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
