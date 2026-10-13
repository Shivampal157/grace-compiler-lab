; RUN: opt -load-pass-plugin %shlibdir/GraceCompilerLab%shlibext \
; RUN:   -passes=grace-print-instruction-mix -disable-output %s 2>&1 | FileCheck %s

define i32 @test_mix(ptr %p, i32 %x) {
entry:
  %v = load i32, ptr %p
  %a = add i32 %v, 0
  %b = mul i32 %a, 1
  %c = add i32 %b, %x
  store i32 %c, ptr %p
  ret i32 %c
}

; CHECK: Function: test_mix
; CHECK: total: 6
; CHECK: memory: 2
; CHECK: loads: 1
; CHECK: stores: 1
; CHECK: arithmetic: 4
