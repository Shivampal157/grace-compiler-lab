cmake_minimum_required(VERSION 3.20)
project(grace-compiler-lab LANGUAGES CXX)

# Support user-provided installation prefix or automatic system discovery
set(LT_LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation prefix")
if(LT_LLVM_INSTALL_DIR)
  list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm" "${LT_LLVM_INSTALL_DIR}")
else()
  # Common Linux & package manager defaults
  list(APPEND CMAKE_PREFIX_PATH
    "/usr/lib/llvm-18/lib/cmake/llvm"
    "/usr/lib/llvm-18"
    "/usr/lib/llvm-17/lib/cmake/llvm"
    "/usr/lib/llvm-17"
    "/usr/lib/llvm-16/lib/cmake/llvm"
    "/usr/lib/llvm-16"
  )
endif()

find_package(LLVM REQUIRED CONFIG)
if(LLVM_VERSION_MAJOR LESS 15)
  message(FATAL_ERROR "LLVM 15 or newer is required. Found: ${LLVM_PACKAGE_VERSION}")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")
message(STATUS "LLVM include directories: ${LLVM_INCLUDE_DIRS}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# 1. Unified Grace Compiler Lab plugin
add_library(GraceCompilerLab MODULE
  lib/GracePassPlugin.cpp
  lib/InstructionMix.cpp
  lib/GraceNeoverseAnalysis.cpp
  lib/GraceFMAOptimization.cpp
  lib/GraceLoopStrideAnalysis.cpp
  lib/ArithmeticSimplify.cpp
)
target_include_directories(GraceCompilerLab PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_options(GraceCompilerLab PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(GraceCompilerLab PROPERTIES PREFIX "")

# 2. Standalone Simplify plugin (maintained for backwards compatibility)
add_library(GraceCompilerLabSimplify MODULE
  lib/ArithmeticSimplify.cpp
)
target_include_directories(GraceCompilerLabSimplify PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_options(GraceCompilerLabSimplify PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(GraceCompilerLabSimplify PROPERTIES PREFIX "")

# 3. Testing configuration
include(CTest)
option(BUILD_TESTING "Build and run LIT regression tests" ON)

if(BUILD_TESTING)
  find_program(LLVM_LIT NAMES llvm-lit lit)
  find_program(LLVM_OPT NAMES opt)
  find_program(LLVM_FILECHECK NAMES FileCheck)

  if(LLVM_LIT AND LLVM_OPT AND LLVM_FILECHECK)
    message(STATUS "Found lit: ${LLVM_LIT}")
    message(STATUS "Found opt: ${LLVM_OPT}")
    message(STATUS "Found FileCheck: ${LLVM_FILECHECK}")

    configure_file(test/lit.site.cfg.py.in
                   "${PROJECT_BINARY_DIR}/test/lit.site.cfg.py" @ONLY)

    add_test(NAME grace-compiler-lit
             COMMAND "${LLVM_LIT}" -sv "${CMAKE_CURRENT_SOURCE_DIR}/test")
  else()
    message(WARNING "LIT tools (llvm-lit, opt, FileCheck) not all found. Regression test target skipped.")
  endif()
endif()
