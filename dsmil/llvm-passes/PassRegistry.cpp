// PassRegistry.cpp - Register DSLLVM CPU feature passes with LLVM
//
// Part of the DSLLVM Project
//
//===----------------------------------------------------------------------===//

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"

#include "DsmilBandwidthEstimate.h"
#include "DsmilAIAccelerate.h"
#include "DsmilSpecHardening.h"
#include "DsmilConstantTimeCheck.h"

using namespace llvm;
using namespace llvm::dsmil;

// Command-line options for DSLLVM passes
static cl::opt<bool> EnableAIAccelerate(
    "dsllvm-ai-accelerate",
    cl::desc("Enable AI acceleration with AVX-VNNI"),
    cl::init(false));

static cl::opt<bool> EnableSpecHardening(
    "dsllvm-spec-hard",
    cl::desc("Enable speculation hardening"),
    cl::init(false));

static cl::opt<std::string> SpecHardeningMode(
    "dsllvm-spec-mode",
    cl::desc("Speculation hardening mode (hardware|hybrid|paranoid)"),
    cl::init("hardware"));

static cl::opt<bool> EnableConstantTimeCheck(
    "dsllvm-ct-check",
    cl::desc("Enable constant-time checking for crypto functions"),
    cl::init(false));

static cl::opt<std::string> CPUProfile(
    "dsllvm-cpu-profile",
    cl::desc("CPU profile to use (e.g., mtr-mtl-dsmil)"),
    cl::init(""));

// Parse hardening mode
static SpecHardeningPass::HardeningMode parseHardeningMode(StringRef Mode) {
  if (Mode == "hardware")
    return SpecHardeningPass::Hardware;
  else if (Mode == "hybrid")
    return SpecHardeningPass::Hybrid;
  else if (Mode == "paranoid")
    return SpecHardeningPass::Paranoid;
  else if (Mode == "off")
    return SpecHardeningPass::Off;
  return SpecHardeningPass::Hardware;
}

// Callback for pass builder
static void registerPasses(PassBuilder &PB) {
  // Register module passes
  PB.registerPipelineParsingCallback(
      [](StringRef Name, ModulePassManager &MPM,
         ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "dsmil-bandwidth-estimate") {
          MPM.addPass(BandwidthEstimatePass());
          return true;
        }
        if (Name == "dsmil-ai-accelerate") {
          MPM.addPass(AIAcceleratePass());
          return true;
        }
        if (Name == "dsmil-spec-hardening") {
          auto Mode = parseHardeningMode(SpecHardeningMode);
          MPM.addPass(SpecHardeningPass(Mode));
          return true;
        }
        if (Name == "dsmil-ct-check") {
          MPM.addPass(ConstantTimeCheckPass());
          return true;
        }
        if (Name == "dsmil-default") {
          // Default DSLLVM pipeline
          MPM.addPass(BandwidthEstimatePass());
          if (EnableAIAccelerate)
            MPM.addPass(AIAcceleratePass());
          if (EnableSpecHardening) {
            auto Mode = parseHardeningMode(SpecHardeningMode);
            MPM.addPass(SpecHardeningPass(Mode));
          }
          if (EnableConstantTimeCheck)
            MPM.addPass(ConstantTimeCheckPass());
          return true;
        }
        return false;
      });
}

// Plugin entry point
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "DSLLVM", "v1.0",
          [](PassBuilder &PB) {
            registerPasses(PB);
          }};
}
