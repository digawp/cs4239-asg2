// Uncomment the following 3 lines for debugging
#ifdef NDEBUG
#undef NDEBUG
#endif
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

typedef std::unordered_map<llvm::Value*, Node> NodeMapType;

std::vector<llvm::Value*> traverse_graph(NodeMapType& node_map, Node start_node) {
  std::unordered_map<llvm::Value*, bool> is_visited;
  std::queue<Node> q;
  std::vector<llvm::Value*> result;

  q.push(start_node);
  is_visited[start_node.llvm_value] = true;

  Node cur;
  while (!q.empty()) {
    cur = q.front();
    is_visited[cur.llvm_value] = true;

    if (cur.type == PRIMITIVE) {
      result.push_back(cur.llvm_value);
    }

    for (auto val_ptr : cur.neighbours) {
      #ifndef NDEBUG
      llvm::outs() << "Current: " << *cur.llvm_value << "\n";
      llvm::outs() << "Next: " << *node_map[val_ptr].llvm_value << "\n";
      #endif
      if (!is_visited[val_ptr]) {
        q.push(node_map.find(val_ptr)->second);
      }
    }
    q.pop();
  }
  return result;
}

llvm::Value* check_last_instruction(NodeMapType node_map, llvm::Value* last_val) {
  if (node_map.find(last_val) != node_map.end() &&
    node_map.find(last_val)->second.type == PRIMITIVE) { // if primitive type
    return last_val;
  }
  return nullptr;
}

/**
 * Check if value is an alloca instruction, if yes, check if it is of pointer type.
 * Insert to the node_map afterwards.
 * Do nothing if value alr exist in node_map
 */
void insert_to_map(NodeMapType& node_map, llvm::Value* value) {
  if (node_map.find(value) != node_map.end()) {
    return;
  }
  NodeType type = VALUE;

  if (auto alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(value)) {
    if (!alloca_inst->getAllocatedType()->isPointerTy()) {
      // llvm::outs() << "Value: " << *value << "\n";
      // llvm::outs() << "Type: " << *value->getType() << "\n";
      // std::cout << "Considered primitive" << std::endl;
      type = PRIMITIVE;
    }
  }

  Node node = {
    .type = type,
    .llvm_value = value
  };
  node_map.insert(std::make_pair(value, node));
}

void handle_intermediate_node(
    NodeMapType& node_map, llvm::Value* intermediate, llvm::Value* actual) {
  if (node_map.find(actual) == node_map.end()) {
    // RHS must be a pointer to an existing thing in the stack (for now)
    std::cout << "Check RHS again" << std::endl;
  }
  insert_to_map(node_map, intermediate);
  // Uncomment if decide to tackle listing 3
  // insert_to_map(pointer);
  // llvm::outs() << *get_ptr_inst << " points to " << *pointer << "\n";
  node_map[intermediate].neighbours.push_back(actual);
}

void create_graph(
    NodeMapType& node_map, llvm::Function* fn) {
  #ifndef NDEBUG
  std::cout << "Creating graph" << std::endl;
  #endif

  // Initialize initial nodes: vars explicitly initialized on stack
  for (auto val_ptr = fn->getValueSymbolTable().begin();
      val_ptr != fn->getValueSymbolTable().end(); ++val_ptr) {
    llvm::Value* value = val_ptr->getValue();
    insert_to_map(node_map, value);
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
      insert_to_map(node_map, pointer);
      insert_to_map(node_map, value);
      llvm::outs() << *pointer << " points to " << *value << "\n";
      node_map.find(pointer)->second.neighbours.push_back(value);
    }

    // get_ptr_inst includes getElementPtr and load instructions
    // They are just intermediate values
    if (auto get_ptr_inst = llvm::dyn_cast<llvm::GetElementPtrInst>(&*iit)) {
      llvm::Value* pointer = get_ptr_inst->getPointerOperand();
      handle_intermediate_node(node_map, get_ptr_inst, pointer);
    }

    if (auto get_ptr_inst = llvm::dyn_cast<llvm::LoadInst>(&*iit)) {
      llvm::Value* pointer = get_ptr_inst->getPointerOperand();
      handle_intermediate_node(node_map, get_ptr_inst, pointer);
    }
  }
  // std::cout << "End creating graph" << std::endl;
}

std::vector<llvm::Value*> get_possible_return_values(llvm::Function& fn) {
  std::vector<llvm::Value*> result;
  for (llvm::inst_iterator I = llvm::inst_begin(&fn);
      I != llvm::inst_end(&fn); ++I) {
    if (auto retInst = llvm::dyn_cast<llvm::ReturnInst>(&*I)) {
      result.push_back(retInst->getReturnValue());
    }
  }
  return result;
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
    #ifndef NDEBUG
    std::cout << "Module name: " << M->getModuleIdentifier() << "\n";
    #endif
    NodeMapType node_map;
    for (auto f_it = M->getFunctionList().begin(),
        e = M->getFunctionList().end(); f_it != e; ++f_it) {
      #ifndef NDEBUG
      std::cout << "=====" << std::endl;
      std::cout << "Declared function: " << f_it->getName().str() << std::endl;
      #endif
      llvm::Function& func = *f_it;

      // If return type not pointer, or just a declaration, or has no local
      // variables, skip it
      if (!func.getReturnType()->isPointerTy() ||
          func.getBasicBlockList().empty() ||
          func.getValueSymbolTable().empty()) {
        continue;
      }

      create_graph(node_map, &func);
      std::vector<llvm::Value*> ret_vals = get_possible_return_values(func);

      for (auto val_ptr : ret_vals) {
        Node& starting_node = node_map.find(val_ptr)->second;
        llvm::outs() << "Starting val: " << *starting_node.llvm_value << "\n";

        std::vector<llvm::Value*> leaked_vars =
            traverse_graph(node_map, starting_node);
        for (auto val_ptr : leaked_vars) {
          llvm::Value* val = check_last_instruction(node_map, val_ptr);
          llvm::outs() << "Result: " << *val << "\n";
          if (val) {
            std::cout << "Warning: returning a pointer which points to ";
            std::cout << val->getName().str() << std::endl;
          }
        }
      }
    }
  }

	return 0;
}
