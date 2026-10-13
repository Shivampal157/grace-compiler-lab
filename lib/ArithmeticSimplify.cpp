#include "llvm/IR/PatternMatch.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace {

class ArithmeticSimplifyPass : public PassInfoMixin<ArithmeticSimplifyPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;
    for (BasicBlock &BB : F) {
      for (auto It = BB.begin(); It != BB.end();) {
        Instruction *I = &*It++;
        Value *Replacement = nullptr;
        if (match(I, m_Add(m_Value(Replacement), m_Zero())) ||
            match(I, m_Sub(m_Value(Replacement), m_Zero())) ||
            match(I, m_Mul(m_Value(Replacement), m_One())))
          I->replaceAllUsesWith(Replacement);
        else if (match(I, m_Mul(m_One(), m_Value(Replacement))))
          I->replaceAllUsesWith(Replacement);
        else
          continue;
        I->eraseFromParent();
        Changed = true;
      }
    }
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

PassPluginLibraryInfo getPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GraceCompilerLabSimplify",
          LLVM_VERSION_STRING, [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "grace-arithmetic-simplify") {
                    FPM.addPass(ArithmeticSimplifyPass());
                    return true;
                  }
                  return false;
                });
          }};
}

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getPluginInfo();
}
