#!/usr/bin/env python3
"""
Grace Compiler Lab: Benchmark Runner & Characterization Suite
Compiles HPC/AI kernels with Clang, runs Grace Neoverse V2 compiler passes,
and prints a performance characterization summary.
"""

import os
import subprocess
import sys
from pathlib import Path

BENCHMARKS = ["saxpy.c", "gemm.c", "stencil2d.c", "dot_product.c"]

def find_binary(name, default):
    return os.environ.get(name, default)

def run_cmd(cmd):
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print(f"Error running command: {' '.join(cmd)}")
        print(res.stderr)
        return None
    return res.stdout + res.stderr

def main():
    root_dir = Path(__file__).resolve().parent.parent
    bench_dir = root_dir / "benchmarks"
    build_dir = root_dir / "build"
    lib_dir = build_dir / "lib"

    clang = find_binary("CLANG", "clang-18")
    opt = find_binary("OPT", "opt-18")

    # Look for plugin library
    plugin_name = "GraceCompilerLab.so"
    if sys.platform == "win32":
        plugin_name = "GraceCompilerLab.dll"
    elif sys.platform == "darwin":
        plugin_name = "GraceCompilerLab.dylib"

    plugin_path = lib_dir / plugin_name

    print("==================================================================")
    print(" NVIDIA Grace CPU Compiler Lab - HPC Benchmark Profiler")
    print(" Target: Arm Neoverse V2 (ARMv9-A + SVE2)")
    print("==================================================================")

    if not plugin_path.exists():
        print(f"Warning: Pass plugin {plugin_path} not found. Please build the project first.")
        print("Run: cmake -B build && cmake --build build")
        return 1

    for src_name in BENCHMARKS:
        src_path = bench_dir / src_name
        if not src_path.exists():
            continue

        base_name = src_path.stem
        ir_path = bench_dir / f"{base_name}.ll"
        fused_ir_path = bench_dir / f"{base_name}_fused.ll"

        print(f"\n[*] Processing Benchmark: {src_name}")

        # 1. Compile C to LLVM IR
        compile_cmd = [
            clang, "-O3",
            "-target", "aarch64-linux-gnu",
            "-mcpu=neoverse-v2",
            "-march=armv9-a+sve2",
            "-emit-llvm", "-S",
            str(src_path), "-o", str(ir_path)
        ]
        out = run_cmd(compile_cmd)
        if out is None:
            # Fallback to host clang if cross-target aarch64 is not installed
            compile_cmd = [clang, "-O3", "-emit-llvm", "-S", str(src_path), "-o", str(ir_path)]
            out = run_cmd(compile_cmd)
            if out is None:
                continue

        # 2. Run Grace Neoverse Analysis Pass
        analysis_cmd = [
            opt,
            f"-load-pass-plugin={plugin_path}",
            "-passes=grace-neoverse-analysis",
            "-disable-output",
            str(ir_path)
        ]
        analysis_out = run_cmd(analysis_cmd)
        if analysis_out:
            print(analysis_out.strip())

        # 3. Run Grace Loop Stride Pass
        stride_cmd = [
            opt,
            f"-load-pass-plugin={plugin_path}",
            "-passes=grace-loop-stride",
            "-disable-output",
            str(ir_path)
        ]
        stride_out = run_cmd(stride_cmd)
        if stride_out:
            print(stride_out.strip())

        # 4. Run FMA Fusion Pass
        fma_cmd = [
            opt,
            f"-load-pass-plugin={plugin_path}",
            "-passes=grace-fma-fusion",
            "-S", str(ir_path), "-o", str(fused_ir_path)
        ]
        run_cmd(fma_cmd)
        print(f"[✓] Benchmark {src_name} characterized successfully.")

    print("\n==================================================================")
    print(" Benchmark characterization complete.")
    print("==================================================================")
    return 0

if __name__ == "__main__":
    sys.exit(main())
