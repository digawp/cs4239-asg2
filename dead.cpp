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

// Get declarations and definitions of the specified function_name inside the
// specified Modules.
std::vector<llvm::Function*> getDeclsAndDefs(
    const std::vector<llvm::Module*>& modules, const std::string& fn_name) {
  std::vector<llvm::Function*> res;
  for (auto& mod : modules) {
    if (auto func = mod->getFunction(fn_name)) {
      res.push_back(func);
    }
  }
  return res;
}

int main(int argc, char **argv) {
  llvm::LLVMContext &Context = llvm::getGlobalContext();
  llvm::SMDiagnostic Err;

  // Step (1) Parse the given IR File
  std::vector<llvm::Module*> modules;
  for (int i = 1; i < argc; ++i) {
    llvm::Module *M = ParseIRFile(argv[i], Err, Context);
    if (M == nullptr) {
      fprintf(stderr, "failed to read IR file %s\n", argv[1]);
      continue;
    }
    modules.push_back(M);
  }

  if (modules.size() == 0) {
    // No legit ll file
    fprintf(stderr, "No appropriate IR file supplied\n");
    return 1;
  }

  std::unordered_set<std::string> functions;
  std::queue<std::string> q;

  // Store all functions in a set.
  for (auto& M : modules){
    std::cout << "Module name: " << M->getModuleIdentifier() << "\n";
    for (auto f_it = M->getFunctionList().begin(),
        e = M->getFunctionList().end(); f_it != e; ++f_it) {
      std::cout << "Declared function: " << f_it->getName().str() << std::endl;
      functions.insert(f_it->getName().str());
    }
  }

  // Initialize first function to be visited
  q.push("main");

  while (!q.empty()) {
    std::string function_name = q.front();
    q.pop();

    // We get all the instance of declarations and definitions of a function
    // because we can't differentiate between forward declaration and the actual
    // definition, sadly (TODO: check if this is true!)
    // The expectation is that for forward declaration, it will not go into the
    // nested for loop (because it has no basic block or instruction), and
    // thus the deepest nested for loop will only execute for one of those
    // instances (the definition instance).
    std::vector<llvm::Function*> func_decls_defs =
        getDeclsAndDefs(modules, function_name);
    for (auto& f : func_decls_defs){
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
    } // for f in func_decls_defs
  } // while

  functions.erase("main");

  std::cout << "\nDead function names:" << std::endl;
  for (auto i = functions.begin(); i != functions.end(); ++i) {
    llvm::outs() << *i << "\n";
  }
}
