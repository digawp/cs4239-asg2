#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "llvm/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

enum NodeType { PRIMITIVE, VALUE };

typedef struct {
  NodeType type;
  llvm::Value* llvm_value;
  std::vector<llvm::Value*> neighbours;
} Node;

std::unordered_map<llvm::Value*, Node> node_map;

llvm::Value* traverse_graph(Node start_node) {
  std::queue<Node> q;
  q.push(start_node);
  Node cur;
  while (!q.empty()) {
    cur = q.front();
    llvm::outs() << "Cur val: " << *cur.llvm_value << "\n";
    for (auto next = cur.neighbours.begin(); next != cur.neighbours.end(); next++) {
      llvm::outs() << *node_map[*next].llvm_value << "\n";
      llvm::outs() << "From " << *cur.llvm_value << "\n";
      if (node_map[*next].type == PRIMITIVE) {
        std::cout << "Ketemu primitip" << std::endl;
        break;
      }
      q.push(node_map[*next]);
    }
    q.pop();
  }

  return cur.llvm_value;
}

llvm::Value* check_last_instruction(llvm::Value* last_val) {
  if (node_map[last_val].type == PRIMITIVE) { // if primitive type
    return last_val;
  }
  
  // if (llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(end_node.llvm_value)) {
  //   if (llvm::Function* called_fn = call_inst->getCalledFunction()) {
  //     if (llvm::dyn_cast<llvm::PointerType>(called_fn->getReturnType())) {
  //       return end_node.llvm_value;
  //     }
  //   }
  // }
  return nullptr;
}

void create_graph(llvm::Function* fn) {
  std::cout << "Creating graph" << std::endl;
  // Initialize initial nodes: vars explicitly initialized on stack
  for (auto val_ptr = fn->getValueSymbolTable().begin();
      val_ptr != fn->getValueSymbolTable().end(); ++val_ptr) {
    Node src_node = {
      .type = VALUE,
      .llvm_value = val_ptr->getValue()
    };
    node_map.insert(std::make_pair(src_node.llvm_value, src_node));
  }

  for (llvm::inst_iterator iit = llvm::inst_begin(fn), E = llvm::inst_end(fn); iit != E; ++iit) {
    if (llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(&*iit)) {
      llvm::Value* pointer = store_inst->getPointerOperand();
      llvm::Value* value = store_inst->getValueOperand();

      if (node_map.find(pointer) == node_map.end()) {
        Node src_node = {
          .type = VALUE,
          .llvm_value = pointer
        };
        node_map.insert(std::make_pair(pointer, src_node));
      }

      if (node_map.find(value) == node_map.end()) {
        NodeType type = VALUE;
        llvm::outs() << *value << "\n";
        if (llvm::dyn_cast<llvm::IntegerType>(value->getType())) {
          std::cout << "Primitive!" << std::endl;
          type = PRIMITIVE;
        }
        Node dst_node = {
          .type = type,
          .llvm_value = value
        };
        node_map.insert(std::make_pair(value, dst_node));
      }
      llvm::outs() << *pointer << " points to " << *value << "\n";
      node_map[pointer].neighbours.push_back(value);
    }
  }
  std::cout << "End creating graph" << std::endl;
}

int main(int argc, char const *argv[]) {
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

  for (auto& M : modules){
    std::cout << "Module name: " << M->getModuleIdentifier() << "\n";
    for (auto f_it = M->getFunctionList().begin(),
        e = M->getFunctionList().end(); f_it != e; ++f_it) {
      std::cout << "Declared function: " << f_it->getName().str() << std::endl;
      llvm::Function& func = *f_it;
      if (!(llvm::dyn_cast<llvm::PointerType>(func.getReturnType()))) {
        continue;
      }
      create_graph(&func);
      Node starting_node;
      for (llvm::inst_iterator I = llvm::inst_begin(&func); I != llvm::inst_end(&func); ++I) {
        if (auto retInst = llvm::dyn_cast<llvm::ReturnInst>(&*I)) {
          llvm::Value* retVal = retInst->getReturnValue();
          if (auto loadRetValInst = llvm::dyn_cast<llvm::LoadInst>(retVal)) {
            llvm::Value* actualRetVal = loadRetValInst->getPointerOperand();
            llvm::outs() << "Actual retval: " << *actualRetVal << "\n";
            starting_node = node_map[actualRetVal];
            break;
          }
        }
      }
      llvm::outs() << "Starting val: " << *starting_node.llvm_value << "\n";
      llvm::Value* ending_node = traverse_graph(starting_node);
      // llvm::Value* res = check_last_instruction(ending_node);
      llvm::outs() << "Result: " << *ending_node << "\n";
      std::cout << "Warning: returning a pointer to a variable in a stack." << std::endl;
      std::cout << "Returning " << starting_node.llvm_value->getName().str() << " which points to ";
      std::cout << ending_node->getName().str() << std::endl;
    }
  }

	return 0;
}
