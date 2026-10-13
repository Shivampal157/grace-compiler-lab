#include "GracePasses.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace grace {

PreservedAnalyses GraceFMAFusionPass::run(Function &F, FunctionAnalysisManager &) {
  bool Changed = false;
  Module *M = F.getParent();

  for (BasicBlock &BB : F) {
    for (auto It = BB.begin(); It != BB.end();) {
      Instruction *I = &*It++;

      // We focus on floating-point addition and subtraction
      if (I->getOpcode() != Instruction::FAdd && I->getOpcode() != Instruction::FSub)
        continue;

      Type *Ty = I->getType();
      if (!Ty->isFPOrFPVectorTy())
        continue;

      Value *MulOp0 = nullptr;
      Value *MulOp1 = nullptr;
      Value *Addend = nullptr;
      Instruction *MulInst = nullptr;
      bool IsSub = (I->getOpcode() == Instruction::FSub);
      bool MulIsSubtrahend = false; // c - (a * b)

      // Match (a * b) + c
      if (!IsSub && match(I, m_FAdd(m_Instruction(MulInst), m_Value(Addend))) &&
          match(MulInst, m_FMul(m_Value(MulOp0), m_Value(MulOp1)))) {
        // Matched
      }
      // Match c + (a * b)
      else if (!IsSub && match(I, m_FAdd(m_Value(Addend), m_Instruction(MulInst))) &&
               match(MulInst, m_FMul(m_Value(MulOp0), m_Value(MulOp1)))) {
        // Matched
      }
      // Match (a * b) - c  =>  (a * b) + (-c)
      else if (IsSub && match(I, m_FSub(m_Instruction(MulInst), m_Value(Addend))) &&
               match(MulInst, m_FMul(m_Value(MulOp0), m_Value(MulOp1)))) {
        // Matched
      }
      // Match c - (a * b)  =>  (-a * b) + c
      else if (IsSub && match(I, m_FSub(m_Value(Addend), m_Instruction(MulInst))) &&
               match(MulInst, m_FMul(m_Value(MulOp0), m_Value(MulOp1)))) {
        MulIsSubtrahend = true;
      }
      else {
        continue;
      }

      IRBuilder<> Builder(I);
      // Transfer fast-math flags if present
      if (auto *FPI = dyn_cast<FPMathOperator>(I))
        Builder.setFastMathFlags(FPI->getFastMathFlags());

      Function *FMulAddFn = Intrinsic::getDeclaration(M, Intrinsic::fmuladd, {Ty});
      Value *FusedCall = nullptr;

      if (!IsSub) {
        // Standard FMA: (a * b) + c
        FusedCall = Builder.CreateCall(FMulAddFn, {MulOp0, MulOp1, Addend});
      } else if (!MulIsSubtrahend) {
        // (a * b) - c => fmuladd(a, b, -c)
        Value *NegAddend = Builder.CreateFNeg(Addend);
        FusedCall = Builder.CreateCall(FMulAddFn, {MulOp0, MulOp1, NegAddend});
      } else {
        // c - (a * b) => fmuladd(-a, b, c)
        Value *NegMulOp0 = Builder.CreateFNeg(MulOp0);
        FusedCall = Builder.CreateCall(FMulAddFn, {NegMulOp0, MulOp1, Addend});
      }

      I->replaceAllUsesWith(FusedCall);
      I->eraseFromParent();

      // If MulInst was only used by this instruction, clean it up
      if (MulInst && MulInst->use_empty())
        MulInst->eraseFromParent();

      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace grace
