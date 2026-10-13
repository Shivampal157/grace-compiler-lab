#include "GracePasses.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace grace {

enum class StrideKind {
  UnitStride,       // Consecutive element access (step = 1) -> SVE2 vector load friendly
  ConstantStride,   // Fixed strided access (step > 1) -> Cache line underutilization
  LoopInvariant,    // Accessing scalar constant or invariant pointer -> Broadcast candidate
  IndirectGather    // Indirect indexing (array[indices[i]]) -> High-latency gather/scatter
};

struct StrideEntry {
  const Value *Ptr;
  bool IsStore;
  StrideKind Kind;
  int64_t Step;
};

PreservedAnalyses GraceLoopStridePass::run(Function &F, FunctionAnalysisManager &) {
  SmallVector<StrideEntry, 16> Strides;
  std::size_t UnitCount = 0;
  std::size_t StridedCount = 0;
  std::size_t IndirectCount = 0;

  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
      const Value *Ptr = nullptr;
      bool IsStore = false;

      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        Ptr = LI->getPointerOperand();
        IsStore = false;
      } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
        Ptr = SI->getPointerOperand();
        IsStore = true;
      } else {
        continue;
      }

      StrideKind Kind = StrideKind::LoopInvariant;
      int64_t Step = 0;

      if (auto *GEP = dyn_cast<GetElementPtrInst>(Ptr)) {
        // Analyze the trailing index operand
        unsigned NumIndices = GEP->getNumIndices();
        if (NumIndices > 0) {
          Value *LastIdx = GEP->getOperand(NumIndices);
          if (auto *CI = dyn_cast<ConstantInt>(LastIdx)) {
            Step = CI->getSExtValue();
            Kind = (Step == 1 || Step == 0) ? StrideKind::UnitStride : StrideKind::ConstantStride;
          } else if (auto *IdxInst = dyn_cast<Instruction>(LastIdx)) {
            if (isa<LoadInst>(IdxInst)) {
              Kind = StrideKind::IndirectGather;
            } else if (IdxInst->getOpcode() == Instruction::Add ||
                       IdxInst->getOpcode() == Instruction::Mul) {
              Kind = StrideKind::UnitStride; // induction variable progression
            } else {
              Kind = StrideKind::ConstantStride;
            }
          } else {
            Kind = StrideKind::UnitStride;
          }
        }
      }

      switch (Kind) {
      case StrideKind::UnitStride:
        ++UnitCount;
        break;
      case StrideKind::ConstantStride:
        ++StridedCount;
        break;
      case StrideKind::IndirectGather:
        ++IndirectCount;
        break;
      default:
        break;
      }

      Strides.push_back({Ptr, IsStore, Kind, Step});
    }
  }

  OS << "-----------------------------------------------------------------\n";
  OS << " Grace LPDDR5X Memory Stride Analysis: " << F.getName() << "\n";
  OS << "-----------------------------------------------------------------\n";
  OS << "   Unit-Stride / Streaming Accesses   : " << UnitCount << " (Prefetch friendly)\n";
  OS << "   Non-Unit Strided Accesses          : " << StridedCount << "\n";
  OS << "   Indirect / Gather-Scatter Accesses : " << IndirectCount << "\n";

  if (IndirectCount > 0) {
    OS << "   * [WARNING] Detected " << IndirectCount
       << " indirect memory access(es). On Grace Neoverse V2, SVE2 gather/scatter\n"
       << "     instructions incur multi-cycle latency. Consider data layout restructuring.\n";
  } else if (UnitCount > 0 && StridedCount == 0) {
    OS << "   * [EXCELLENT] Contiguous streaming pattern. Hardware prefetchers and\n"
       << "     SVE2 contiguous vector loads (ld1w/ld1d) will saturate LPDDR5X channels.\n";
  }
  OS << "-----------------------------------------------------------------\n";

  return PreservedAnalyses::all();
}

} // namespace grace
