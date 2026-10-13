; RUN: opt -load-pass-plugin %shlibdir/GraceCompilerLab%shlibext \
; RUN:   -passes=grace-fma-fusion -S %s | FileCheck %s

define float @scalar_fma(float %a, float %b, float %c) {
entry:
  %m = fmul float %a, %b
  %res = fadd float %m, %c
  ret float %res
}

; CHECK-LABEL: @scalar_fma
; CHECK-NOT: fmul float
; CHECK-NOT: fadd float
; CHECK: call float @llvm.fmuladd.f32(float %a, float %b, float %c)

define <4 x float> @vector_fma(<4 x float> %a, <4 x float> %b, <4 x float> %c) {
entry:
  %m = fmul <4 x float> %a, %b
  %res = fadd <4 x float> %m, %c
  ret <4 x float> %res
}

; CHECK-LABEL: @vector_fma
; CHECK-NOT: fmul <4 x float>
; CHECK-NOT: fadd <4 x float>
; CHECK: call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %a, <4 x float> %b, <4 x float> %c)

define double @fma_sub(double %a, double %b, double %c) {
entry:
  %m = fmul double %a, %b
  %res = fsub double %m, %c
  ret double %res
}

; CHECK-LABEL: @fma_sub
; CHECK-NOT: fsub double
; CHECK: call double @llvm.fmuladd.f64
