#include "ir.h"
#include <string>

IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t,
                 const std::vector<std::string> &params)
    : block(bb_), op(op), outType(t), params(params) {}

void IRInstr::genAsm(std::ostream &os,
                     std::map<std::string, Symbol> &symbolTable) {
  switch (op) {
  case const_assign:
    os << "movl $" << params[1] << ", -" << symbolTable[params[0]].offset
       << "(%rbp)"
       << "\n";
    break;
  case var_assign:
    os << "movl -" << symbolTable[params[1]].offset << "(%rbp)"
       << ", %eax"
       << "\n";
    os << "movl %eax"
       << ", -" << symbolTable[params[0]].offset << "(%rbp)"
       << "\n";
    break;
  case ret:
    os << "    movl $" << params[0] << ", %eax\n";
    os << "popq %rbp\n";
    os << "ret\n";
  }
}

std::ostream &operator<<(std::ostream &os, IRInstr &instruction) {
  if (instruction.op == IRInstr::Operation::add) {
    os << instruction.params[0] << " = " << instruction.params[1] << " + "
       << instruction.params[2];
  } else if (instruction.op == IRInstr::Operation::const_assign) {
    os << instruction.params[0] << " = " << instruction.params[1];
  } else if (instruction.op == IRInstr::Operation::var_assign) {
    os << instruction.params[0] << " = " << instruction.params[1];
  } else if (instruction.op == IRInstr::Operation::ret) {
    os << "ret " << instruction.params[0];
  }

  else {
    os << "Operation display not supported";
  }
  // TODO: Implement printing for other operations
  return os;
}

BasicBlock::BasicBlock(CFG *cfg, std::string entry_label)
    : cfg(cfg), label(std::move(entry_label)) {}

void BasicBlock::gen_asm(std::ostream &o,
                         std::map<std::string, Symbol> &symbolTable) {
  for (auto &instruction : instrs) {
    instruction.genAsm(o, symbolTable);
  }
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t,
                             const std::vector<std::string> &params) {
  instrs.emplace_back(this, op, t, params);
}

CFG::~CFG() {
  for (auto bb : bbs) {
    delete bb;
  }
}

void CFG::add_bb(BasicBlock *bb) {
  bbs.push_back(bb);
  current_bb = bb;
}

std::string CFG::IR_reg_to_asm(std::string reg) {
  // TODO
  return "";
}

void CFG::gen_asm_prologue(std::ostream &o) {
  o << ".globl main\n";
  o << " main: \n";

  o << "pushq %rbp\n";
  o << "movq %rsp, %rbp\n";
}

void CFG::gen_asm(std::ostream &o) {
  gen_asm_prologue(o);
  current_bb->gen_asm(o, symbolTable);
  gen_asm_epilogue(o);
}
void CFG::gen_asm_epilogue(std::ostream &o) {
  // TODO
}

void CFG::add_to_symbol_table(std::string name, Type t, int line) {
  Symbol s;
  s.type = t;
  s.offset = nextFreeSymbolIndex;
  symbolTable[name] = s;
  nextFreeSymbolIndex += 4;
}
std::string CFG::create_new_tempvar(Type t, int line) {
  std::string tempVarName = "!T" + std::to_string(nextFreeSymbolIndex);
  add_to_symbol_table(tempVarName, t, line);
  return tempVarName;
}
int CFG::get_var_index(std::string name) {
  // TODO
  return 0;
}
Type CFG::get_var_type(std::string name) {
  // TODO
  return Type::INT;
}
