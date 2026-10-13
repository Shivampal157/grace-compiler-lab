#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"

namespace grace {

// HPC Fused Multiply-Add (FMA) and Arithmetic Contraction Pass
// Targets NVIDIA Grace CPU's 4x128-bit vector pipelines by contracting
// (a * b) + c and (a * b) - c patterns into target-optimal llvm.fmuladd / llvm.fma.
class GraceFMAFusionPass : public llvm::PassInfoMixin<GraceFMAFusionPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
};

// Memory Access Stride Analysis Pass
// Classifies memory access patterns within loops to assess cache-line streaming
// and prefetching efficiency for Grace's 546 GB/s LPDDR5X subsystem.
class GraceLoopStridePass : public llvm::PassInfoMixin<GraceLoopStridePass> {
public:
  explicit GraceLoopStridePass(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);

private:
  llvm::raw_ostream &OS;
};

// Extended Algebraic Simplification & Strength Reduction Pass
// Safely folds identity and neutral elements (x+0, x-0, x*1, x*0, x^x, x&0, x|0)
// and rewrites power-of-two multiplication into shifts.
class GraceArithmeticSimplifyPass : public llvm::PassInfoMixin<GraceArithmeticSimplifyPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
};

llvm::PassPluginLibraryInfo getGracePluginInfo();

} // namespace grace
