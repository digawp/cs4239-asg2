#include <iostream>
#include <unordered_set>
#include <queue>

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
  std::queue<std::string> q;

  // Store all functions in a set.

  for (auto f_it = M->getFunctionList().begin(),
      e = M->getFunctionList().end(); f_it != e; ++f_it) {
    std::cout << "Declared function: " << f_it->getName().str() << std::endl;
    functions.insert(f_it->getName().str());
  }

  q.push("main");

  while (!q.empty()) {
    std::string function_name = q.front();
    q.pop();
    llvm::Function *f = M->getFunction(function_name);

    for (auto &bb : *f) {
      for (auto &i : bb) {
        if (llvm::CallInst* callInst = llvm::dyn_cast<llvm::CallInst>(&i)) {
          std::string called_function_name =
              callInst->getCalledFunction()->getName().str();
          // Remove function from functions set if it is called.
          if (functions.erase(called_function_name) == 1) {
            q.push(called_function_name);
          }
        } // if isa<CallInst>(i)
      } // for i in bb
    } // for bb in *f
  } // while

  functions.erase("main");

  std::cout << "\nDead function names:" << std::endl;
  for (auto i = functions.begin(); i != functions.end(); ++i) {
    llvm::outs() << *i;
    std::cout << std::endl;
  }
}
