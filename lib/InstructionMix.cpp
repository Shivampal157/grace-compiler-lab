#include "InstructionMix.h"

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace grace {

AnalysisKey InstructionMixAnalysis::Key;

std::size_t InstructionMixResult::total() const {
  std::size_t Sum = 0;
  for (std::size_t Count : counts)
    Sum += Count;
  return Sum;
}

std::size_t InstructionMixResult::memory_operations() const {
  return counts[static_cast<unsigned>(InstructionBucket::Load)] +
         counts[static_cast<unsigned>(InstructionBucket::Store)];
}

void InstructionMixResult::print(raw_ostream &OS) const {
  OS << "GRACE-LAB instruction mix\n";
  OS << "  total: " << total() << "\n";
  OS << "  memory: " << memory_operations() << "\n";
  OS << "  loads: " << counts[static_cast<unsigned>(InstructionBucket::Load)] << "\n";
  OS << "  stores: " << counts[static_cast<unsigned>(InstructionBucket::Store)] << "\n";
  OS << "  calls: " << counts[static_cast<unsigned>(InstructionBucket::Call)] << "\n";
  OS << "  branches: " << counts[static_cast<unsigned>(InstructionBucket::Branch)] << "\n";
  OS << "  arithmetic: " << counts[static_cast<unsigned>(InstructionBucket::Arithmetic)] << "\n";
}

InstructionMixAnalysis::Result
InstructionMixAnalysis::run(Function &F, FunctionAnalysisManager &) {
  Result Result;
  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
      InstructionBucket Bucket = InstructionBucket::Arithmetic;
      if (isa<LoadInst>(I))
        Bucket = InstructionBucket::Load;
      else if (isa<StoreInst>(I))
        Bucket = InstructionBucket::Store;
      else if (isa<CallBase>(I))
        Bucket = InstructionBucket::Call;
      else if (isa<BranchInst>(I) || isa<SwitchInst>(I))
        Bucket = InstructionBucket::Branch;
      ++Result.counts[static_cast<unsigned>(Bucket)];
    }
  }
  return Result;
}

PreservedAnalyses InstructionMixPrinter::run(Function &F,
                                             FunctionAnalysisManager &FAM) {
  OS << "Function: " << F.getName() << "\n";
  FAM.getResult<InstructionMixAnalysis>(F).print(OS);
  return PreservedAnalyses::all();
}

PassPluginLibraryInfo getInstructionMixPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GraceCompilerLab",
          LLVM_VERSION_STRING, [](PassBuilder &PB) {
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([] { return InstructionMixAnalysis(); });
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "grace-print-instruction-mix") {
                    FPM.addPass(InstructionMixPrinter(errs()));
                    return true;
                  }
                  return false;
                });
          }};
}

} // namespace grace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return grace::getInstructionMixPluginInfo();
}
