#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

#include <array>
#include <cstddef>
#include <string>

namespace grace {

// Microarchitectural execution port classification modeled after
// the Arm Neoverse V2 core (NVIDIA Grace CPU).
//
// Neoverse V2 key pipeline characteristics:
// - 16-wide decode, 8-wide dispatch/rename
// - 4x 128-bit SIMD/Vector execution pipes (V0, V1, V2, V3) supporting SVE2 & NEON
// - 6 Integer ALU units (I0 - I5)
// - 3 Load ports (L0, L1, L2)
// - 2 Store ports (S0, S1)
// - 2 Branch execution units (B0, B1)
// - Peak throughput: up to 4 vector ops / cycle, 3 loads / cycle, 2 stores / cycle
enum class NeoversePort {
  VectorFP,   // Ports V0-V3 (SVE2 / NEON / FP compute, 4 per cycle)
  IntALU,     // Ports I0-I5 (Integer arithmetic, logic, shifts, 6 per cycle)
  Load,       // Ports L0-L2 (Load units, 3 per cycle)
  Store,      // Ports S0-S1 (Store units, 2 per cycle)
  Branch,     // Ports B0-B1 (Branch units, 2 per cycle)
  Other,      // Other control / pseudo instructions
  Count
};

enum class BottleneckKind {
  ComputeVector, // Vector/FP unit throughput limit
  ComputeInt,    // Integer ALU throughput limit
  MemoryLoad,    // Load unit port pressure
  MemoryStore,   // Store unit port pressure
  BranchControl, // Branch frequency saturation
  Balanced       // Well-distributed across execution units
};

struct NeoverseModelMetrics {
  std::size_t TotalInstructions = 0;
  std::size_t VectorOps = 0;
  std::size_t ScalableVectorOps = 0; // SVE / SVE2 scalable vectors
  std::size_t FixedVectorOps = 0;    // NEON / Fixed-width vectors
  std::size_t ScalarFPOps = 0;
  std::size_t ScalarIntOps = 0;
  std::size_t Loads = 0;
  std::size_t Stores = 0;
  std::size_t Atomics = 0;           // Arm LSE atomic instructions
  std::size_t Branches = 0;
  std::size_t Calls = 0;
  std::size_t FusedMulAddCandidates = 0;

  // Port instruction distributions
  std::array<std::size_t, static_cast<unsigned>(NeoversePort::Count)> PortCounts{};

  // Computed metrics
  double getVectorizationRatio() const; // VectorOps / (VectorOps + ScalarCompute)
  double getMemoryIntensity() const;   // (Loads + Stores) / TotalInstructions
  double getEstimatedMinCycles() const;
  double getEstimatedPeakIPC() const;
  BottleneckKind getPrimaryBottleneck() const;
  const char *getBottleneckName() const;

  void print(llvm::raw_ostream &OS, llvm::StringRef FunctionName) const;
};

// Neoverse V2 Architecture Analysis Pass
class GraceNeoverseAnalysis : public llvm::AnalysisInfoMixin<GraceNeoverseAnalysis> {
public:
  using Result = NeoverseModelMetrics;
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);

  static llvm::AnalysisKey Key;
};

// Printer pass for opt pipeline integration
class GraceNeoversePrinter : public llvm::PassInfoMixin<GraceNeoversePrinter> {
public:
  explicit GraceNeoversePrinter(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);

private:
  llvm::raw_ostream &OS;
};

} // namespace grace
