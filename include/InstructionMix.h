#pragma once

#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

#include <array>
#include <cstddef>

namespace grace {

enum class InstructionBucket {
  Load,
  Store,
  Call,
  Branch,
  Arithmetic,
  VectorSIMD,
  AtomicLSE,
  Count
};

struct InstructionMixResult {
  std::array<std::size_t, static_cast<unsigned>(InstructionBucket::Count)> counts{};
  std::size_t total() const;
  std::size_t memory_operations() const;
  std::size_t vector_operations() const;
  void print(llvm::raw_ostream &OS) const;
};

class InstructionMixAnalysis
    : public llvm::AnalysisInfoMixin<InstructionMixAnalysis> {
public:
  using Result = InstructionMixResult;
  Result run(llvm::Function &, llvm::FunctionAnalysisManager &);

  static llvm::AnalysisKey Key;
};

class InstructionMixPrinter
    : public llvm::PassInfoMixin<InstructionMixPrinter> {
public:
  explicit InstructionMixPrinter(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);

private:
  llvm::raw_ostream &OS;
};

} // namespace grace
