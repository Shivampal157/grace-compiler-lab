#!/usr/bin/env python3
"""
Grace Analyst CLI Tool
Interactive command-line diagnostic tool for analyzing compiler workloads
on NVIDIA Grace CPU (Arm Neoverse V2) architecture.
"""

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

import sys
if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

def draw_bar(val, max_val=100, width=30):
    clamped = max(0.0, min(float(val), float(max_val)))
    ratio = clamped / float(max_val) if max_val > 0 else 0
    filled = int(round(ratio * width))
    bar = "=" * filled + "-" * (width - filled)
    return f"[{bar}] {val:5.1f}%"

def analyze_ir_text(ir_text):
    total = 0
    loads = len(re.findall(r"\bload\s+", ir_text))
    stores = len(re.findall(r"\bstore\s+", ir_text))
    calls = len(re.findall(r"\bcall\s+", ir_text))
    branches = len(re.findall(r"\b(br|switch|ret)\s+", ir_text))
    vectors = len(re.findall(r"<(?:\d+|vscale\s*x\s*\d+)\s*x\s*(?:float|double|i\d+)>", ir_text))
    fmul = len(re.findall(r"\bfmul\s+", ir_text))
    fadd = len(re.findall(r"\bfadd\s+", ir_text))
    fsub = len(re.findall(r"\bfsub\s+", ir_text))
    fma_intrinsics = len(re.findall(r"@llvm\.fmuladd|@llvm\.fma", ir_text))
    atomics = len(re.findall(r"\b(atomicrmw|cmpxchg)\b", ir_text))

    for line in ir_text.splitlines():
        line = line.strip()
        if line and not line.startswith(";") and not line.startswith("define") and "=" in line:
            total += 1

    total = max(total, loads + stores + calls + branches + fmul + fadd + fsub + fma_intrinsics)
    if total == 0:
        total = 1

    vec_ratio = (vectors / total) * 100.0 if total > 0 else 0.0
    mem_intensity = ((loads + stores) / total) * 100.0 if total > 0 else 0.0

    # Neoverse V2 cycle bounds:
    # 4 vector ops/cycle, 3 loads/cycle, 2 stores/cycle, 8 dispatch limit
    vec_cycles = vectors / 4.0
    load_cycles = loads / 3.0
    store_cycles = stores / 2.0
    dispatch_cycles = total / 8.0
    est_cycles = max(1.0, vec_cycles, load_cycles, store_cycles, dispatch_cycles)
    est_ipc = min(8.0, total / est_cycles)

    bottleneck = "Balanced"
    if max(vec_cycles, load_cycles, store_cycles) == vec_cycles and vec_cycles > 0:
        bottleneck = "COMPUTE-BOUND (Vector Units V0-V3 saturated)"
    elif max(vec_cycles, load_cycles, store_cycles) == load_cycles and load_cycles > 0:
        bottleneck = "MEMORY-BOUND (Load Port L0-L2 saturation)"
    elif max(vec_cycles, load_cycles, store_cycles) == store_cycles and store_cycles > 0:
        bottleneck = "MEMORY-BOUND (Store Port S0-S1 saturation)"

    return {
        "total": total,
        "loads": loads,
        "stores": stores,
        "calls": calls,
        "branches": branches,
        "vectors": vectors,
        "fmul": fmul,
        "fadd": fadd,
        "fsub": fsub,
        "fma": fma_intrinsics,
        "atomics": atomics,
        "vec_ratio": vec_ratio,
        "mem_intensity": mem_intensity,
        "est_cycles": est_cycles,
        "est_ipc": est_ipc,
        "bottleneck": bottleneck,
    }

def print_dashboard(metrics, source_name):
    print("\n" + "=" * 70)
    print("   NVIDIA GRACE CPU (Arm Neoverse V2) ARCHITECTURAL DASHBOARD")
    print(f"   Target: Grace Hopper / Grace CPU Superchip | File: {source_name}")
    print("=" * 70)
    print(f" Total Instructions Analyzed  : {metrics['total']:>6}")
    print(f" Memory Loads / Stores        : {metrics['loads']:>4} loads / {metrics['stores']:>4} stores")
    print(f" SIMD / Vector Operations     : {metrics['vectors']:>6}")
    print(f" Fused Multiply-Add Calls     : {metrics['fma']:>6}")
    print(f" Control Flow (Branches/Calls): {metrics['branches']:>4} branches / {metrics['calls']:>4} calls")
    print("-" * 70)
    print(" PIPELINE UTILIZATION & RESOURCE PRESSURE")
    print(f"   Vectorization Ratio : {draw_bar(metrics['vec_ratio'])}")
    print(f"   Memory Intensity    : {draw_bar(metrics['mem_intensity'])}")
    print("-" * 70)
    print(f" Estimated Throughput IPC : {metrics['est_ipc']:4.2f} / 8.00 (Neoverse V2 Peak)")
    print(f" Est. Pipeline Execution : {metrics['est_cycles']:4.1f} cycles")
    print(f" Primary Bottleneck      : {metrics['bottleneck']}")
    print("=" * 70)
    print(" COMPILER TUNING ADVICE FOR NVIDIA GRACE:")
    if metrics['fmul'] > 0 and metrics['fadd'] > 0 and metrics['fma'] == 0:
        print("  [!] Unfused fmul/fadd found. Add -ffp-contract=fast or grace-fma-fusion pass.")
    if metrics['vec_ratio'] < 20.0 and metrics['total'] > 15:
        print("  [!] Low vectorization ratio. Enable -march=armv9-a+sve2 -mcpu=neoverse-v2.")
    if metrics['mem_intensity'] > 40.0:
        print("  [!] Memory bound kernel. Ensure unit-stride access for LPDDR5X channels.")
    if metrics['atomics'] > 0:
        print("  [✓] Arm LSE atomics in use (optimal for 72-core Grace scalability).")
    print("=" * 70 + "\n")

def main():
    parser = argparse.ArgumentParser(description="Grace Compiler Lab Architectural Analyst")
    parser.add_argument("input", help="Path to LLVM IR (.ll) or C source file (.c)")
    args = parser.parse_args()

    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: File {input_path} not found.")
        sys.exit(1)

    if input_path.suffix == ".ll":
        with open(input_path, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()
        metrics = analyze_ir_text(content)
        print_dashboard(metrics, input_path.name)
    elif input_path.suffix == ".c":
        print(f"Compiling {input_path.name} to LLVM IR...")
        ir_out = input_path.with_suffix(".ll")
        try:
            subprocess.run(["clang", "-O3", "-emit-llvm", "-S", str(input_path), "-o", str(ir_out)], check=True)
            with open(ir_out, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
            metrics = analyze_ir_text(content)
            print_dashboard(metrics, input_path.name)
        except Exception as e:
            print(f"Could not invoke clang automatically: {e}")
            print("Please provide a .ll file directly.")

if __name__ == "__main__":
    main()
