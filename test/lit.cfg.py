import os
import lit.formats

config.name = "GraceCompilerLab"
config.test_format = lit.formats.ShTest(True)
config.suffixes = [".ll"]

if not hasattr(config, "test_source_root") or not config.test_source_root:
    config.test_source_root = os.path.dirname(__file__)

if not hasattr(config, "test_exec_root") or not config.test_exec_root:
    config.test_exec_root = config.test_source_root

shlibdir = getattr(config, "llvm_shlib_dir", "")
shlibext = getattr(config, "llvm_shlib_ext", ".so")
opt_bin = getattr(config, "opt", "opt")
filecheck_bin = getattr(config, "filecheck", "FileCheck")

config.substitutions.append(("%shlibdir", shlibdir))
config.substitutions.append(("%shlibext", shlibext))
config.substitutions.append(("opt", opt_bin))
config.substitutions.append(("FileCheck", filecheck_bin))
