cmake_minimum_required(VERSION 3.20)
project(grace-compiler-lab LANGUAGES CXX)

set(LT_LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation prefix")
if(NOT EXISTS "${LT_LLVM_INSTALL_DIR}/include/llvm")
  message(FATAL_ERROR "Set LT_LLVM_INSTALL_DIR to an LLVM installation, e.g. C:/LLVM")
endif()

list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm")
find_package(LLVM CONFIG REQUIRED)
if(LLVM_VERSION_MAJOR LESS 15)
  message(FATAL_ERROR "LLVM 15 or newer is required")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(GraceCompilerLab MODULE
  lib/InstructionMix.cpp
)
target_compile_options(GraceCompilerLab PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(GraceCompilerLab PROPERTIES PREFIX "")

add_library(GraceCompilerLabSimplify MODULE
  lib/ArithmeticSimplify.cpp
)
target_compile_options(GraceCompilerLabSimplify PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(GraceCompilerLabSimplify PROPERTIES PREFIX "")

include(CTest)
if(BUILD_TESTING)
  find_program(LLVM_LIT NAMES llvm-lit lit REQUIRED)
  find_program(LLVM_OPT NAMES opt REQUIRED)
  find_program(LLVM_FILECHECK NAMES FileCheck REQUIRED)
  configure_file(test/lit.site.cfg.py.in
                 "${PROJECT_BINARY_DIR}/test/lit.site.cfg.py" @ONLY)
  add_test(NAME grace-compiler-lit
           COMMAND "${LLVM_LIT}" -sv "${PROJECT_BINARY_DIR}/test")
endif()
