include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/test
)

set (test_sources
  ign_TEST.cc
)

if (HAVE_IGN_TOOLS)
  ign_build_tests(TYPE UNIT SOURCES ${test_sources})
endif ()
