// Uncomment the following 3 lines for debugging
// #ifdef NDEBUG
// #undef NDEBUG
// #endif
#include <cassert>
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


/**
 * @brief      Stores all functions name in the modules into the set given.
 *
 * @param      fn_set   The function set
 * @param      modules  The modules
 *
 * @return true if there is at least one main function in the modules, false
 * otherwise.
 */
bool store_functions(std::unordered_set<std::string>& fn_set,
                     std::vector<llvm::Module*>& modules) {
  bool has_main = false;
  for (auto& M : modules){
    #ifndef NDEBUG
    std::cout << "Module name: " << M->getModuleIdentifier() << "\n";
    #endif
    for (auto f_it = M->getFunctionList().begin(),
        e = M->getFunctionList().end(); f_it != e; ++f_it) {
      std::string fn_name = f_it->getName().str();
      has_main = has_main ? has_main : fn_name == "main";
      #ifndef NDEBUG
      std::cout << "Declared function: " << fn_name << std::endl;
      #endif
      fn_set.insert(fn_name);
    }
  }
  return has_main;
}

/**
 * @brief      Gets the llvm::Function* of fn_name.
 *
 * This method assumes that there is only one definition per function name
 * (which is the case for C). If not, it returns the first function
 * definition found in the list of modules.
 *
 * @param[in]  modules  List of all modules
 * @param[in]  fn_name  The function name
 *
 * @return NULL if fn_name does not exist in any of the modules
 */
llvm::Function* get_function_definition(
    const std::vector<llvm::Module*>& modules, const std::string& fn_name) {
  for (auto& mod : modules) {
    auto func = mod->getFunction(fn_name);
    if (func && !func->isDeclaration()) {
      return func;
    }
  }
  return NULL;
}

/**
 * @brief      Gets the module name from function name given.
 *
 * Assumption: the function definition exists in one of the modules.
 *
 * @param[in]  modules  The modules
 * @param[in]  fn_name  The function name
 *
 * @return     The module name/identifier of the function.
 */
std::string get_module_name_from_fn_name(
    const std::vector<llvm::Module*>& modules, const std::string& fn_name) {
  llvm::Function* fn = get_function_definition(modules, fn_name);
  assert(fn);
  return fn->getParent()->getModuleIdentifier();
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
    fprintf(stderr, "No appropriate IR file supplied\n");
    return 1;
  }

  // Store all functions in a set. The function will be removed from the set if
  // it is found to be used when we trace the call graph.
  std::unordered_set<std::string> functions;
  bool has_main = store_functions(functions, modules);

  // Initialize queue of functions to be traced
  std::queue<std::string> q;
  if (has_main) {
    q.push("main");
  }

  // Skip the whole loop is there is no main
  while (!q.empty()) {
    std::string function_name = q.front();
    q.pop();

    llvm::Function* f = get_function_definition(modules, function_name);

    // f should never be NULL
    assert(f);

    // Iterate through all instructions in the function.
    for (auto& bb : *f) {
      for (auto& inst : bb) {
        for (auto op_idx = 0; op_idx < inst.getNumOperands(); ++op_idx) {
          auto val = inst.getOperand(op_idx);
          if (llvm::dyn_cast<llvm::Function>(val)) {
            std::string used_function_name = val->getName().str();
            // Remove function from the functions set if it is used by
            // the instruction.
            if (functions.erase(used_function_name) == 1) {
              q.push(used_function_name);
            }
          } // if dyn_cast<Function>
        } // for all operands
      } // for inst in bb
    } // for bb in *f
  } // while

  functions.erase("main");

  std::cout << "Dead function names:" << std::endl;
  for (auto i = functions.begin(); i != functions.end(); ++i) {
    llvm::outs() << *i << " in " << get_module_name_from_fn_name(modules, *i) << "\n";
  }
}
