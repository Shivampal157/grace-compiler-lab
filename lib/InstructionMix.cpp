#include "InstructionMix.h"

#include "llvm/IR/Instructions.h"
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

std::size_t InstructionMixResult::vector_operations() const {
  return counts[static_cast<unsigned>(InstructionBucket::VectorSIMD)];
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
  if (counts[static_cast<unsigned>(InstructionBucket::VectorSIMD)] > 0)
    OS << "  vector_simd: " << counts[static_cast<unsigned>(InstructionBucket::VectorSIMD)] << "\n";
  if (counts[static_cast<unsigned>(InstructionBucket::AtomicLSE)] > 0)
    OS << "  atomic_lse: " << counts[static_cast<unsigned>(InstructionBucket::AtomicLSE)] << "\n";
}

InstructionMixAnalysis::Result
InstructionMixAnalysis::run(Function &F, FunctionAnalysisManager &) {
  Result Res;
  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
      if (I.getType()->isVectorTy()) {
        ++Res.counts[static_cast<unsigned>(InstructionBucket::VectorSIMD)];
        continue;
      }
      if (isa<AtomicRMWInst>(I) || isa<AtomicCmpXchgInst>(I)) {
        ++Res.counts[static_cast<unsigned>(InstructionBucket::AtomicLSE)];
        continue;
      }
      if (isa<LoadInst>(I)) {
        ++Res.counts[static_cast<unsigned>(InstructionBucket::Load)];
      } else if (isa<StoreInst>(I)) {
        ++Res.counts[static_cast<unsigned>(InstructionBucket::Store)];
      } else if (isa<CallBase>(I)) {
        ++Res.counts[static_cast<unsigned>(InstructionBucket::Call)];
      } else if (isa<BranchInst>(I) || isa<SwitchInst>(I)) {
        ++Res.counts[static_cast<unsigned>(InstructionBucket::Branch)];
      } else {
        ++Res.counts[static_cast<unsigned>(InstructionBucket::Arithmetic)];
      }
    }
  }
  return Res;
}

PreservedAnalyses InstructionMixPrinter::run(Function &F,
                                             FunctionAnalysisManager &FAM) {
  OS << "Function: " << F.getName() << "\n";
  FAM.getResult<InstructionMixAnalysis>(F).print(OS);
  return PreservedAnalyses::all();
}

} // namespace grace
