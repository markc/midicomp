# CMakeLists.txt v0.0.6 20110727 (C) markc@renta.net (MIT)

cmake_minimum_required(VERSION 3.10)
project(midicomp C)

# This is pre-ANSI K&R C; build it as gnu89 so modern compilers accept the
# empty-parens prototypes and implicit declarations.
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_EXTENSIONS ON)

include_directories(${CMAKE_SOURCE_DIR})
set(midicomp_executable_SRCS
  yyread.c
  midicomp.c
  t2mflex.c
)
set(midicomp_executable_HDRS
  midifile.h
  midicomp.h
  t2mf.h
)
add_executable(midicomp ${midicomp_executable_SRCS})
install(TARGETS midicomp DESTINATION bin)

# --- Tests (run with: cmake .. && make && ctest) -----------------------------
enable_testing()

set(_midicomp_test_driver "${CMAKE_SOURCE_DIR}/tests/run_test.cmake")
foreach(mode plain verbose roundtrip canonical)
  add_test(
    NAME ${mode}
    COMMAND ${CMAKE_COMMAND}
      -DBIN=$<TARGET_FILE:midicomp>
      -DSRCDIR=${CMAKE_SOURCE_DIR}
      -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}
      -DMODE=${mode}
      -P ${_midicomp_test_driver})
endforeach()
