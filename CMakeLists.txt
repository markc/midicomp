# CMakeLists.txt v0.0.6 20110727 (C) markc@renta.net (AGPLv3)

project(midicomp)
cmake_minimum_required(VERSION 2.6)
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
