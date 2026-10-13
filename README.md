# Grace Compiler Lab

[![Build](https://github.com/Shivampal157/grace-compiler-lab/actions/workflows/build.yml/badge.svg)](https://github.com/Shivampal157/grace-compiler-lab/actions/workflows/build.yml)

An original, focused LLVM project for studying CPU compiler engineering. The
project uses LLVM's New Pass Manager to inspect instruction mix and apply a
small semantics-preserving arithmetic transformation. It is designed to make
compiler decisions measurable and explainable rather than to claim access to
NVIDIA Grace compiler internals.

## Why this project

CPU compiler performance depends on more than source-level algorithms. A useful
compiler engineer must reason about instruction selection, memory intensity,
control flow, data dependencies, and the cost of transforming IR. This lab
provides a compact example of that workflow:

```text
LLVM IR -> analysis -> conservative transformation -> FileCheck regression test
```

## Passes

### `grace-print-instruction-mix`

An LLVM function analysis that classifies instructions into:

- loads and stores;
- calls;
- branches and switches; and
- arithmetic/other instructions.

It reports total instruction count and memory-operation intensity. The result
is an IR-level diagnostic, not a claim about wall-clock speed on a specific
CPU.

### `grace-arithmetic-simplify`

A conservative transformation that removes identity arithmetic:

```text
x + 0 -> x
x - 0 -> x
x * 1 -> x
1 * x -> x
```

The pass uses LLVM's pattern-matching utilities, replaces uses safely, removes
the redundant instruction, and invalidates analyses when the function changes.

## Repository layout

```text
include/      analysis result API
lib/          out-of-tree LLVM pass implementations
inputs/       small LLVM IR regression input
benchmarks/   source kernels for future target/codegen experiments
test/         LIT configuration
```

## Build on Linux

The most reproducible environment is Ubuntu with LLVM development packages:

```bash
sudo apt-get update
sudo apt-get install -y cmake ninja-build llvm-18 llvm-18-dev llvm-18-tools clang-18
cmake -S . -B build -G Ninja \
  -DLT_LLVM_INSTALL_DIR=/usr/lib/llvm-18 \
  -DBUILD_TESTING=OFF
cmake --build build
```

The project accepts LLVM 15 or newer. On another LLVM version, replace the
package and installation-prefix paths accordingly.

Run the IR verifier:

```bash
/usr/lib/llvm-18/bin/opt -passes=verify -disable-output inputs/kernel.ll
```

Run the analysis plugin after building:

```bash
/usr/lib/llvm-18/bin/opt \
  -load-pass-plugin ./build/lib/GraceCompilerLab.so \
  -passes=grace-print-instruction-mix \
  -disable-output inputs/kernel.ll
```

Run the transformation plugin:

```bash
/usr/lib/llvm-18/bin/opt \
  -load-pass-plugin ./build/lib/GraceCompilerLabSimplify.so \
  -passes=grace-arithmetic-simplify \
  -S inputs/kernel.ll -o -
```

## Testing

The checked-in IR contains `RUN` lines for both plugins. With `llvm-lit` and
`FileCheck` installed, run:

```bash
llvm-lit -sv test
```

The GitHub Actions workflow builds the project and verifies the input IR on
Ubuntu. Local Windows builds may require a complete LLVM development
distribution containing C++ headers and `LLVMConfig.cmake`; a Clang-only
installation is not sufficient.

## Interview discussion

- Why is instruction mix useful but insufficient to predict runtime?
- Why are loads/stores not treated as common subexpressions here?
- Which analyses become invalid after replacing and erasing instructions?
- How would an AArch64 or RISC-V target model change the cost heuristic?
- How would loop trip counts and register pressure extend this lab?

## Scope and attribution

This repository is an original implementation using public LLVM APIs. It is not
copied from `llvm-tutor` and does not reproduce NVIDIA proprietary code or
Grace compiler internals. LLVM is used as the compiler infrastructure; see
the LLVM project license and documentation for the upstream APIs.
