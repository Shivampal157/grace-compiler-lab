; RUN: opt -load-pass-plugin %shlibdir/GraceCompilerLab%shlibext \
; RUN:   -passes=grace-neoverse-analysis -disable-output %s 2>&1 | FileCheck %s

define void @vector_kernel(ptr %dst, ptr %src, i32 %n) {
entry:
  %v0 = load <4 x float>, ptr %src
  %v1 = fadd <4 x float> %v0, %v0
  %v2 = fmul <4 x float> %v1, %v0
  store <4 x float> %v2, ptr %dst
  ret void
}

; CHECK: NVIDIA Grace CPU (Arm Neoverse V2) Microarchitecture Report
; CHECK: Function: vector_kernel
; CHECK: Total Instructions
; CHECK: Vector / SIMD Ops
; CHECK: Memory Loads
; CHECK: Memory Stores
; CHECK: Vectorization Ratio
; CHECK: Est. Throughput IPC
; CHECK: Primary Bottleneck
