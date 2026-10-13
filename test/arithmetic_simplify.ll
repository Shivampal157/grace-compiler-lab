; RUN: opt -load-pass-plugin %shlibdir/GraceCompilerLab%shlibext \
; RUN:   -passes=grace-arithmetic-simplify -S %s | FileCheck %s

define i32 @simplify_identities(i32 %x) {
entry:
  %a = add i32 %x, 0
  %b = mul i32 %a, 1
  %c = sub i32 %b, 0
  ret i32 %c
}

; CHECK-LABEL: @simplify_identities
; CHECK-NOT: add i32 %x, 0
; CHECK-NOT: mul i32
; CHECK: ret i32 %x

define i32 @simplify_zero(i32 %x) {
entry:
  %z = mul i32 %x, 0
  ret i32 %z
}

; CHECK-LABEL: @simplify_zero
; CHECK: ret i32 0

define i32 @simplify_xor_self(i32 %x) {
entry:
  %x2 = xor i32 %x, %x
  ret i32 %x2
}

; CHECK-LABEL: @simplify_xor_self
; CHECK: ret i32 0

define i32 @strength_reduce_mul(i32 %x) {
entry:
  %m = mul i32 %x, 8
  ret i32 %m
}

; CHECK-LABEL: @strength_reduce_mul
; CHECK-NOT: mul i32 %x, 8
; CHECK: shl i32 %x, 3
