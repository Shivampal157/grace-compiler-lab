#include "GracePasses.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace grace {

PreservedAnalyses GraceArithmeticSimplifyPass::run(Function &F, FunctionAnalysisManager &) {
  bool Changed = false;

  for (BasicBlock &BB : F) {
    for (auto It = BB.begin(); It != BB.end();) {
      Instruction *I = &*It++;
      Value *X = nullptr;
      ConstantInt *CI = nullptr;

      // 1. Identity additions and subtractions: x + 0 -> x, x - 0 -> x, x * 1 -> x, 1 * x -> x
      if (match(I, m_Add(m_Value(X), m_Zero())) ||
          match(I, m_Sub(m_Value(X), m_Zero())) ||
          match(I, m_Mul(m_Value(X), m_One())) ||
          match(I, m_Mul(m_One(), m_Value(X)))) {
        I->replaceAllUsesWith(X);
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // 2. Integer multiplication by zero: x * 0 -> 0
      if (match(I, m_Mul(m_Value(X), m_Zero())) ||
          match(I, m_Mul(m_Zero(), m_Value(X)))) {
        Constant *Zero = Constant::getNullValue(I->getType());
        I->replaceAllUsesWith(Zero);
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // 3. Bitwise XOR self: x ^ x -> 0
      if (I->getOpcode() == Instruction::Xor && I->getOperand(0) == I->getOperand(1)) {
        Constant *Zero = Constant::getNullValue(I->getType());
        I->replaceAllUsesWith(Zero);
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // 4. Bitwise AND zero: x & 0 -> 0
      if (match(I, m_And(m_Value(X), m_Zero())) ||
          match(I, m_And(m_Zero(), m_Value(X)))) {
        Constant *Zero = Constant::getNullValue(I->getType());
        I->replaceAllUsesWith(Zero);
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // 5. Bitwise OR zero: x | 0 -> x
      if (match(I, m_Or(m_Value(X), m_Zero())) ||
          match(I, m_Or(m_Zero(), m_Value(X)))) {
        I->replaceAllUsesWith(X);
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // 6. Bitwise XOR zero: x ^ 0 -> x
      if (match(I, m_Xor(m_Value(X), m_Zero())) ||
          match(I, m_Xor(m_Zero(), m_Value(X)))) {
        I->replaceAllUsesWith(X);
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // 7. Bitwise idempotence: x & x -> x, x | x -> x
      if ((I->getOpcode() == Instruction::And || I->getOpcode() == Instruction::Or) &&
          I->getOperand(0) == I->getOperand(1)) {
        I->replaceAllUsesWith(I->getOperand(0));
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // 8. Strength reduction: x * 2^k -> x << k
      if (match(I, m_Mul(m_Value(X), m_ConstantInt(CI))) && CI->getValue().isPowerOf2()) {
        unsigned ShiftAmt = CI->getValue().logBase2();
        IRBuilder<> Builder(I);
        Value *ShiftVal = ConstantInt::get(I->getType(), ShiftAmt);
        Value *Shl = Builder.CreateShl(X, ShiftVal);
        I->replaceAllUsesWith(Shl);
        I->eraseFromParent();
        Changed = true;
        continue;
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace grace
