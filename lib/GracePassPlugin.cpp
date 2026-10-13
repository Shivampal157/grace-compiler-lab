#include "GraceNeoverseModel.h"
#include "GracePasses.h"
#include "InstructionMix.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace grace {

PassPluginLibraryInfo getGracePluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION,
      "GraceCompilerLab",
      LLVM_VERSION_STRING,
      [](PassBuilder &PB) {
        // Register analyses with FunctionAnalysisManager
        PB.registerAnalysisRegistrationCallback(
            [](FunctionAnalysisManager &FAM) {
              FAM.registerPass([] { return InstructionMixAnalysis(); });
              FAM.registerPass([] { return GraceNeoverseAnalysis(); });
            });

        // Register pass names for opt pipeline parsing (-passes=...)
        PB.registerPipelineParsingCallback(
            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "grace-print-instruction-mix") {
                FPM.addPass(InstructionMixPrinter(errs()));
                return true;
              }
              if (Name == "grace-print-neoverse-analysis" || Name == "grace-neoverse-analysis") {
                FPM.addPass(GraceNeoversePrinter(errs()));
                return true;
              }
              if (Name == "grace-fma-fusion") {
                FPM.addPass(GraceFMAFusionPass());
                return true;
              }
              if (Name == "grace-loop-stride") {
                FPM.addPass(GraceLoopStridePass(errs()));
                return true;
              }
              if (Name == "grace-arithmetic-simplify") {
                FPM.addPass(GraceArithmeticSimplifyPass());
                return true;
              }
              return false;
            });
      }};
}

} // namespace grace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return grace::getGracePluginInfo();
}
