# CMakeLists.txt v0.2.0 20260613 (C) markc@renta.net (MIT)

cmake_minimum_required(VERSION 3.10)
project(midicomp VERSION 0.2.0 LANGUAGES C)

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

# Our hand-written sources (midicomp.c, yyread.c) compile warning-clean under -Wall.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(midicomp PRIVATE -Wall)
  # t2mflex.c is flex-generated; flex always emits char-subscript table lookups
  # and unused input()/yyunput() helpers. Suppress only those on that one file.
  set_source_files_properties(t2mflex.c PROPERTIES
    COMPILE_OPTIONS "-Wno-char-subscripts;-Wno-unused-function")
endif()

install(TARGETS midicomp DESTINATION bin)

# --- Tests (run with: cmake .. && make && ctest) -----------------------------
enable_testing()

set(_midicomp_test_driver "${CMAKE_SOURCE_DIR}/tests/run_test.cmake")
foreach(mode plain verbose roundtrip canonical smpte security)
  add_test(
    NAME ${mode}
    COMMAND ${CMAKE_COMMAND}
      -DBIN=$<TARGET_FILE:midicomp>
      -DSRCDIR=${CMAKE_SOURCE_DIR}
      -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}
      -DMODE=${mode}
      -P ${_midicomp_test_driver})
endforeach()
