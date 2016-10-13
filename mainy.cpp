#include <iostream>
#include <unordered_set>

#include "llvm/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

void printAllFunctions(std::unordered_set<std::string> functions) {
  for (auto it = functions.begin(); it != functions.end(); ++it) {
    std::cout << *it << std::endl;
  }
}

int main(int argc, char **argv) {
  // Step (1) Parse the given IR File
  llvm::LLVMContext &Context = llvm::getGlobalContext();
  llvm::SMDiagnostic Err;
  llvm::Module *M = ParseIRFile(argv[1], Err, Context);
  if (M == nullptr) {
    fprintf(stderr, "failed to read IR file %s\n", argv[1]);
    return 1;
  }

  std::unordered_set<std::string> functions;

  for (auto f_it = M->getFunctionList().begin(),
      e = M->getFunctionList().end(); f_it != e; ++f_it) {
    std::cout << "(Supposedly) fn name: " << f_it->getName().str() << std::endl;
    functions.insert(f_it->getName().str());
  }
  
  // Step (2) Check main exists
  std::unordered_set<std::string>::const_iterator search = functions.find("main");
  if (search == functions.end()) {
    std::cout << "No main function" << std::endl;
    // print_all_functions(functions);
    return 0;
  }
  
  // Step (3) Traverse all instructions
  for (auto &F : *M) { // For each function F
    llvm::outs() << F.getName() << "\n";
    for (auto &bb : F) {
      llvm::outs() << "BB: " << bb << "\n";
      for (auto &i : bb) {
        if (llvm::CallInst* callInst = llvm::dyn_cast<llvm::CallInst>(&i)) {
          std::cout << "Found called function: " << callInst->getCalledFunction()->getName().str() << std::endl;
          functions.erase(callInst->getCalledFunction()->getName().str());
        }
      }
    }
  }
  
  // Assumption: main() always gets called, hence, we exclude main function 
  // from the output
  functions.erase("main");
  std::cout << "Dead:\n";
  printAllFunctions(functions);
  
  // for (auto i = functions.begin(); i != functions.end(); ++i) {
  //   llvm::outs() << *i;
  //   std::cout << std::endl;
  // }
  return 0;
}