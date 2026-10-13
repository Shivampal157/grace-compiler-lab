import lit.formats
import lit.util
import os

config.name = "GraceCompilerLab"
config.test_format = lit.formats.ShTest(True)
config.suffixes = [".ll"]
config.test_source_root = os.path.join("@CMAKE_CURRENT_SOURCE_DIR@", "..", "inputs")
config.test_exec_root = os.path.join("@CMAKE_BINARY_DIR@", "test")
config.substitutions.append(("%shlibdir", "@CMAKE_LIBRARY_OUTPUT_DIRECTORY@"))
config.substitutions.append(("%shlibext", "@CMAKE_SHARED_LIBRARY_SUFFIX@"))
config.substitutions.append(("opt", "@LLVM_OPT@"))
config.substitutions.append(("FileCheck", "@LLVM_FILECHECK@"))
