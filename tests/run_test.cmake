# run_test.cmake - CTest driver for midicomp (pure CMake, no shell needed)
#
# Invoked per-test as:
#   cmake -DBIN=<midicomp> -DSRCDIR=<repo> -DWORKDIR=<build> -DMODE=<mode> -P run_test.cmake
#
# Modes:
#   plain      decode ex1.mid              == ex1-plain.txt   (golden)
#   verbose    decode -v -t ex1.mid        == ex1-verbose.txt (golden)
#   roundtrip  text -> SMF -> text is stable (idempotent decode)
#   canonical  midicomp's SMF output is byte-stable on re-compile

function(run)
  # run(<result-var> <args...>) - execute midicomp, FATAL on non-zero exit
  cmake_parse_arguments(A "" "OUT;IN" "ARGS" ${ARGN})
  set(extra "")
  if(A_OUT)
    set(extra OUTPUT_FILE "${A_OUT}")
  endif()
  if(A_IN)
    list(APPEND extra INPUT_FILE "${A_IN}")
  endif()
  execute_process(
    COMMAND "${BIN}" ${A_ARGS}
    ${extra}
    RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "midicomp ${A_ARGS} exited with ${rc}")
  endif()
endfunction()

function(must_match expected actual what)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${expected}" "${actual}"
    RESULT_VARIABLE d)
  if(NOT d EQUAL 0)
    message(FATAL_ERROR "${what}: ${actual} does not match ${expected}")
  endif()
endfunction()

if(MODE STREQUAL "plain")
  run(ARGS "${SRCDIR}/ex1.mid" OUT "${WORKDIR}/plain.txt")
  must_match("${SRCDIR}/ex1-plain.txt" "${WORKDIR}/plain.txt" "plain decode")

elseif(MODE STREQUAL "verbose")
  run(ARGS -v -t "${SRCDIR}/ex1.mid" OUT "${WORKDIR}/verbose.txt")
  must_match("${SRCDIR}/ex1-verbose.txt" "${WORKDIR}/verbose.txt" "verbose decode")

elseif(MODE STREQUAL "roundtrip")
  # decode -> text -> recompile -> decode again; the two decodes must be identical
  run(ARGS "${SRCDIR}/ex1.mid" OUT "${WORKDIR}/a.txt")
  run(ARGS -c "${WORKDIR}/rt.mid" IN "${WORKDIR}/a.txt")
  run(ARGS "${WORKDIR}/rt.mid" OUT "${WORKDIR}/b.txt")
  must_match("${WORKDIR}/a.txt" "${WORKDIR}/b.txt" "text round-trip")

elseif(MODE STREQUAL "canonical")
  # once midicomp has written an SMF, re-compiling its decode is byte-stable.
  # (the original ex1.mid uses running status, so gen-1 is intentionally larger)
  run(ARGS "${SRCDIR}/ex1.mid" OUT "${WORKDIR}/c1.txt")
  run(ARGS -c "${WORKDIR}/c1.mid" IN "${WORKDIR}/c1.txt")
  run(ARGS "${WORKDIR}/c1.mid" OUT "${WORKDIR}/c2.txt")
  run(ARGS -c "${WORKDIR}/c2.mid" IN "${WORKDIR}/c2.txt")
  must_match("${WORKDIR}/c1.mid" "${WORKDIR}/c2.mid" "canonical SMF stability")

elseif(MODE STREQUAL "smpte")
  # SMPTE-division headers (MFile ... -fps ticks) must compile and round-trip.
  # Regression guard: a too-strict division bound once rejected valid SMPTE.
  run(ARGS -c "${WORKDIR}/smpte1.mid" IN "${SRCDIR}/tests/fixtures/smpte.txt")
  run(ARGS "${WORKDIR}/smpte1.mid" OUT "${WORKDIR}/smpte1.txt")
  must_match("${SRCDIR}/tests/fixtures/smpte.txt" "${WORKDIR}/smpte1.txt" "SMPTE round-trip")

elseif(MODE STREQUAL "security")
  # Adversarial inputs that previously crashed (NULL deref, OOB read, SIGFPE)
  # or triggered UB. Assert midicomp handles each WITHOUT crashing: a clean
  # exit is 0 (handled) or 1 (reported error); a signal (SIGSEGV/SIGFPE) shows
  # up as a non-numeric RESULT_VARIABLE or a code >= 128.
  function(no_crash what)
    # remaining args after `what` are the midicomp argv
    execute_process(
      COMMAND "${BIN}" ${ARGN}
      OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE rc)
    if(NOT rc MATCHES "^[01]$")
      message(FATAL_ERROR "CRASH/abnormal exit on ${what}: midicomp ${ARGN} -> '${rc}'")
    endif()
    message(STATUS "  no-crash: ${what} (exit ${rc})")
  endfunction()

  set(fx "${SRCDIR}/tests/fixtures")
  # decode path (also under -t which exercises the prtime division guards)
  foreach(f meta-tempo-len0 meta-smpte-len0 meta-keysig-len0 header-division0 huge-varlen)
    no_crash("decode ${f}" "${fx}/${f}.mid")
    no_crash("decode -t ${f}" -t "${fx}/${f}.mid")
  endforeach()
  # compile path with out-of-range / malformed values.
  # NB: two-arg compile is `-c <input text> <output midi>`, so the fixture
  # (input) MUST come first, the output path second.
  foreach(f compile-value-oob compile-timesig-denom0 compile-hex-oob)
    no_crash("compile ${f}" -c "${fx}/${f}.txt" "${WORKDIR}/${f}.out.mid")
  endforeach()

else()
  message(FATAL_ERROR "unknown MODE '${MODE}'")
endif()

message(STATUS "PASS: ${MODE}")
