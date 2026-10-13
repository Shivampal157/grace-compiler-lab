; RUN: opt -load-pass-plugin %shlibdir/GraceCompilerLab%shlibext \
; RUN:   -passes=grace-loop-stride -disable-output %s 2>&1 | FileCheck %s

define void @streaming_saxpy(ptr %x, ptr %y, float %a, i32 %n) {
entry:
  %p0 = getelementptr inbounds float, ptr %x, i64 1
  %v0 = load float, ptr %p0
  %p1 = getelementptr inbounds float, ptr %y, i64 1
  store float %v0, ptr %p1
  ret void
}

; CHECK: Grace LPDDR5X Memory Stride Analysis: streaming_saxpy
; CHECK: Unit-Stride / Streaming Accesses
; CHECK: Contiguous streaming pattern
