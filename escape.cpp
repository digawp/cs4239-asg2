#include <iostream>
#include <queue>
#include <unordered_map>

#include "llvm/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
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

std::vector<llvm::Value*> traverse_graph(Node start_node) {
  std::vector<llvm::Value*> result;
  std::queue<Node> q;
  q.push(start_node);
  Node cur;
  while (!q.empty()) {
    cur = q.front();
    if (cur.type == PRIMITIVE) {
      result.push_back(cur.llvm_value);
    }
    for (auto next = cur.neighbours.begin(); next != cur.neighbours.end(); next++) {
      // llvm::outs() << "Current: " << *cur.llvm_value << "\n";
      // llvm::outs() << "Next: " << *node_map[*next].llvm_value << "\n";
      q.push(node_map.find(*next)->second);
    }
    q.pop();
  }
  return result;
}

llvm::Value* check_last_instruction(llvm::Value* last_val) {
  if (node_map[last_val].type == PRIMITIVE) { // if primitive type
    return last_val;
  }
  return nullptr;
}

/**
 * Check if value is an alloca instruction, if yes, check if it is of pointer type.
 * Insert to the node_map afterwards.
 * Do nothing if value alr exist in node_map
 */
void insert_to_map(llvm::Value* value) {
  if (node_map.find(value) != node_map.end()) {
    return;
  }
  NodeType type = VALUE;

  if (auto alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(value)) {
    if (!alloca_inst->getAllocatedType()->isPointerTy()) {
      llvm::outs() << "Value: " << *value << "\n";
      llvm::outs() << "Type: " << *value->getType() << "\n";
      std::cout << "Considered non-pointer" << std::endl;
      type = PRIMITIVE;
    }
  }

  Node node = {
    .type = type,
    .llvm_value = value
  };
  node_map.insert(std::make_pair(value, node));
}

void create_graph(llvm::Function* fn) {
  std::cout << "Creating graph" << std::endl;

  // Initialize initial nodes: vars explicitly initialized on stack
  for (auto val_ptr = fn->getValueSymbolTable().begin();
      val_ptr != fn->getValueSymbolTable().end(); ++val_ptr) {
    llvm::Value* value = val_ptr->getValue();
    insert_to_map(value);
  }

  for (llvm::inst_iterator iit = llvm::inst_begin(fn), E = llvm::inst_end(fn); iit != E; ++iit) {
    if (llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(&*iit)) {
      llvm::Value* pointer = store_inst->getPointerOperand();
      llvm::Value* value = store_inst->getValueOperand();

      auto val_node_pair = node_map.find(pointer);
      if (val_node_pair != node_map.end() &&
          val_node_pair->second.type == PRIMITIVE) {
        // Already primitive type, skip to the next instruction
        continue;
      }
      insert_to_map(pointer);
      insert_to_map(value);
      llvm::outs() << *pointer << " points to " << *value << "\n";
      node_map[pointer].neighbours.push_back(value);
    }

    // get_ptr_inst includes getElementPtr and load instructions
    // They are just intermediate values
    if (auto get_ptr_inst = llvm::dyn_cast<llvm::GetElementPtrInst>(&*iit)) {
      llvm::Value* pointer = get_ptr_inst->getPointerOperand();
      if (node_map.find(pointer) == node_map.end()) {
        // RHS must be a pointer to an existing thing in the stack (for now)
        std::cout << "Check RHS again" << std::endl;
      }
      insert_to_map(get_ptr_inst);
      // Uncomment if decide to tackle listing 3
      // insert_to_map(pointer);
      llvm::outs() << *get_ptr_inst << " points to " << *pointer << "\n";
      node_map[get_ptr_inst].neighbours.push_back(pointer);
    }

    if (auto get_ptr_inst = llvm::dyn_cast<llvm::LoadInst>(&*iit)) {
      llvm::Value* pointer = get_ptr_inst->getPointerOperand();
      if (node_map.find(pointer) == node_map.end()) {
        // RHS must be a pointer to an existing thing in the stack (for now)
        std::cout << "Check RHS again" << std::endl;
      }
      insert_to_map(get_ptr_inst);
      // Uncomment if decide to tackle listing 3
      // insert_to_map(pointer);
      llvm::outs() << *get_ptr_inst << " points to " << *pointer << "\n";
      node_map[get_ptr_inst].neighbours.push_back(pointer);
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
      if (!func.getReturnType()->isPointerTy() ||
          func.getBasicBlockList().empty() ||
          func.getValueSymbolTable().empty()) {
        continue;
      }
      create_graph(&func);

      for (llvm::inst_iterator I = llvm::inst_begin(&func);
          I != llvm::inst_end(&func); ++I) {
        if (auto retInst = llvm::dyn_cast<llvm::ReturnInst>(&*I)) {
          llvm::Value* retVal = retInst->getReturnValue();
          Node& starting_node = node_map.find(retVal)->second;
          llvm::outs() << "Starting val: " << *starting_node.llvm_value << "\n";
          
          std::vector<llvm::Value*> leaked_vars = traverse_graph(starting_node);
          for (auto val_ptr : leaked_vars) {
            llvm::Value* val = check_last_instruction(val_ptr);
            llvm::outs() << "Result: " << *val << "\n";
            if (val) {
              std::cout << "Warning: returning a pointer which points to ";
              std::cout << val->getName().str() << std::endl;
            }
          }
        }
      }
    }
  }

	return 0;
}
