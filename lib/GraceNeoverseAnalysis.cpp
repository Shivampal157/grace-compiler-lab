#include "GraceNeoverseModel.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cmath>

using namespace llvm;

namespace grace {

AnalysisKey GraceNeoverseAnalysis::Key;

double NeoverseModelMetrics::getVectorizationRatio() const {
  std::size_t TotalCompute = VectorOps + ScalarFPOps + ScalarIntOps;
  if (TotalCompute == 0)
    return 0.0;
  return (static_cast<double>(VectorOps) / static_cast<double>(TotalCompute)) * 100.0;
}

double NeoverseModelMetrics::getMemoryIntensity() const {
  if (TotalInstructions == 0)
    return 0.0;
  return (static_cast<double>(Loads + Stores) / static_cast<double>(TotalInstructions)) * 100.0;
}

double NeoverseModelMetrics::getEstimatedMinCycles() const {
  if (TotalInstructions == 0)
    return 0.0;

  // Modeling Arm Neoverse V2 pipeline issue rates:
  // - 4 Vector/FP ops per cycle (V0, V1, V2, V3)
  // - 3 Loads per cycle (L0, L1, L2)
  // - 2 Stores per cycle (S0, S1)
  // - 6 Int ALUs per cycle (I0 - I5)
  // - 2 Branches per cycle (B0, B1)
  // - 8 instructions max dispatch/rename per cycle
  double VectorCycles = static_cast<double>(VectorOps + ScalarFPOps) / 4.0;
  double LoadCycles   = static_cast<double>(Loads) / 3.0;
  double StoreCycles  = static_cast<double>(Stores) / 2.0;
  double IntCycles    = static_cast<double>(ScalarIntOps) / 6.0;
  double BranchCycles = static_cast<double>(Branches) / 2.0;
  double DispatchCycles = static_cast<double>(TotalInstructions) / 8.0;

  double MaxCycles = std::max({VectorCycles, LoadCycles, StoreCycles, IntCycles, BranchCycles, DispatchCycles});
  return MaxCycles < 1.0 ? 1.0 : MaxCycles;
}

double NeoverseModelMetrics::getEstimatedPeakIPC() const {
  double MinCycles = getEstimatedMinCycles();
  if (MinCycles <= 0.0)
    return 0.0;
  double IPC = static_cast<double>(TotalInstructions) / MinCycles;
  return std::min(IPC, 8.0); // Clamped at 8-wide dispatch limit
}

BottleneckKind NeoverseModelMetrics::getPrimaryBottleneck() const {
  if (TotalInstructions == 0)
    return BottleneckKind::Balanced;

  double VectorPressure = static_cast<double>(VectorOps + ScalarFPOps) / 4.0;
  double LoadPressure   = static_cast<double>(Loads) / 3.0;
  double StorePressure  = static_cast<double>(Stores) / 2.0;
  double BranchPressure = static_cast<double>(Branches) / 2.0;
  double IntPressure    = static_cast<double>(ScalarIntOps) / 6.0;

  double MaxPressure = std::max({VectorPressure, LoadPressure, StorePressure, BranchPressure, IntPressure});

  if (MaxPressure == 0.0)
    return BottleneckKind::Balanced;
  if (MaxPressure == VectorPressure)
    return BottleneckKind::ComputeVector;
  if (MaxPressure == LoadPressure)
    return BottleneckKind::MemoryLoad;
  if (MaxPressure == StorePressure)
    return BottleneckKind::MemoryStore;
  if (MaxPressure == BranchPressure)
    return BottleneckKind::BranchControl;
  return BottleneckKind::ComputeInt;
}

const char *NeoverseModelMetrics::getBottleneckName() const {
  switch (getPrimaryBottleneck()) {
  case BottleneckKind::ComputeVector:
    return "Vector/FP Execution Units (Ports V0-V3 saturated)";
  case BottleneckKind::ComputeInt:
    return "Integer ALUs (Ports I0-I5 saturated)";
  case BottleneckKind::MemoryLoad:
    return "Load Units (Ports L0-L2 throughput limit)";
  case BottleneckKind::MemoryStore:
    return "Store Units (Ports S0-S1 throughput limit)";
  case BottleneckKind::BranchControl:
    return "Branch Execution Units (Ports B0-B1 saturated)";
  case BottleneckKind::Balanced:
    return "Well balanced across Neoverse V2 execution ports";
  }
  return "Unknown";
}

void NeoverseModelMetrics::print(raw_ostream &OS, StringRef FunctionName) const {
  OS << "=================================================================\n";
  OS << " NVIDIA Grace CPU (Arm Neoverse V2) Microarchitecture Report\n";
  OS << " Function: " << FunctionName << "\n";
  OS << "=================================================================\n";
  OS << " [Instruction Distribution]\n";
  OS << "   Total Instructions       : " << TotalInstructions << "\n";
  OS << "   - Vector / SIMD Ops      : " << VectorOps;
  if (ScalableVectorOps > 0)
    OS << " (" << ScalableVectorOps << " SVE/SVE2 scalable, " << FixedVectorOps << " NEON)";
  OS << "\n";
  OS << "   - Scalar FP Arithmetic   : " << ScalarFPOps << "\n";
  OS << "   - Scalar Integer ALU     : " << ScalarIntOps << "\n";
  OS << "   - Memory Loads           : " << Loads << "\n";
  OS << "   - Memory Stores          : " << Stores << "\n";
  OS << "   - Atomics (Arm LSE)      : " << Atomics << "\n";
  OS << "   - Branches / Switches    : " << Branches << "\n";
  OS << "   - Calls                  : " << Calls << "\n";
  OS << "   - FMA Opportunities      : " << FusedMulAddCandidates << "\n";
  OS << "\n";
  OS << " [Neoverse V2 Pipeline Metrics]\n";
  OS << "   Vectorization Ratio      : " << format("%5.2f", getVectorizationRatio()) << "%\n";
  OS << "   Memory Intensity         : " << format("%5.2f", getMemoryIntensity()) << "%\n";
  OS << "   Est. Execution Latency   : " << format("%5.1f", getEstimatedMinCycles()) << " cycles\n";
  OS << "   Est. Throughput IPC      : " << format("%5.2f", getEstimatedPeakIPC()) << " / 8.00\n";
  OS << "   Primary Bottleneck       : " << getBottleneckName() << "\n";
  OS << "-----------------------------------------------------------------\n";

  // Actionable compiler feedback
  OS << " [Compiler Optimization Insights]\n";
  if (FusedMulAddCandidates > 0) {
    OS << "   * [PASS-OPPORTUNITY] Found " << FusedMulAddCandidates
       << " unfused multiply-add chain(s). Apply grace-fma-fusion pass\n"
       << "     or compile with -ffp-contract=fast to utilize Grace 4x128b FMA units.\n";
  }
  if (getVectorizationRatio() < 25.0 && TotalInstructions > 10) {
    OS << "   * [VECTORIZATION] Low vectorization ratio ("
       << format("%4.1f", getVectorizationRatio())
       << "%). Profile loop headers with -mcpu=neoverse-v2 -march=armv9-a+sve2.\n";
  }
  if (getMemoryIntensity() > 40.0) {
    OS << "   * [MEMORY-BANDWIDTH] High memory intensity ("
       << format("%4.1f", getMemoryIntensity())
       << "%). Inspect memory strides with grace-loop-stride for LPDDR5X bandwidth saturation.\n";
  }
  if (Branches > 0 && (static_cast<double>(Branches) / static_cast<double>(TotalInstructions)) > 0.25) {
    OS << "   * [BRANCH-PENALTY] High branch density. Consider loop unrolling or predicated execution.\n";
  }
  OS << "=================================================================\n\n";
}

GraceNeoverseAnalysis::Result
GraceNeoverseAnalysis::run(Function &F, FunctionAnalysisManager &) {
  NeoverseModelMetrics M;

  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
      ++M.TotalInstructions;

      // Check for SVE/NEON vector operations
      Type *Ty = I.getType();
      if (Ty->isVectorTy()) {
        ++M.VectorOps;
        if (isa<ScalableVectorType>(Ty))
          ++M.ScalableVectorOps;
        else
          ++M.FixedVectorOps;
      }

      // Memory operations
      if (isa<LoadInst>(I)) {
        ++M.Loads;
        ++M.PortCounts[static_cast<unsigned>(NeoversePort::Load)];
      } else if (isa<StoreInst>(I)) {
        ++M.Stores;
        ++M.PortCounts[static_cast<unsigned>(NeoversePort::Store)];
      } else if (isa<AtomicRMWInst>(I) || isa<AtomicCmpXchgInst>(I)) {
        ++M.Atomics;
        ++M.PortCounts[static_cast<unsigned>(NeoversePort::Load)];
      } else if (isa<BranchInst>(I) || isa<SwitchInst>(I)) {
        ++M.Branches;
        ++M.PortCounts[static_cast<unsigned>(NeoversePort::Branch)];
      } else if (isa<CallBase>(I)) {
        ++M.Calls;
        ++M.PortCounts[static_cast<unsigned>(NeoversePort::Other)];
      } else if (I.isBinaryOp()) {
        if (Ty->isFPOrFPVectorTy()) {
          ++M.ScalarFPOps;
          ++M.PortCounts[static_cast<unsigned>(NeoversePort::VectorFP)];
        } else {
          ++M.ScalarIntOps;
          ++M.PortCounts[static_cast<unsigned>(NeoversePort::IntALU)];
        }

        // Check if this addition/subtraction could be fused with a preceding multiplication
        if (I.getOpcode() == Instruction::FAdd || I.getOpcode() == Instruction::FSub ||
            I.getOpcode() == Instruction::Add || I.getOpcode() == Instruction::Sub) {
          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);
          auto isMul = [](Value *V) {
            if (auto *Inst = dyn_cast<Instruction>(V))
              return Inst->getOpcode() == Instruction::FMul || Inst->getOpcode() == Instruction::Mul;
            return false;
          };
          if (isMul(Op0) || isMul(Op1))
            ++M.FusedMulAddCandidates;
        }
      } else {
        ++M.PortCounts[static_cast<unsigned>(NeoversePort::IntALU)];
      }
    }
  }

  return M;
}

PreservedAnalyses GraceNeoversePrinter::run(Function &F, FunctionAnalysisManager &FAM) {
  auto &Metrics = FAM.getResult<GraceNeoverseAnalysis>(F);
  Metrics.print(OS, F.getName());
  return PreservedAnalyses::all();
}

} // namespace grace
