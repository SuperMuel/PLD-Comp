#include "ir.h"
#include <string>

IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t,
                 const std::vector<std::string> &params)
    : block(bb_), op(op), outType(t), params(params) {}

void IRInstr::genAsm(std::ostream &os) {
  // TODO
}

std::ostream &operator<<(std::ostream &os, IRInstr &instruction) {
  if (instruction.op == IRInstr::Operation::add) {
    os << instruction.params[0] << " = " << instruction.params[1] << " + "
       << instruction.params[2];
  } else {
    os << "Operation display not supported";
  }
  // TODO: Implement printing for other operations
  return os;
}

BasicBlock::BasicBlock(CFG *cfg, std::string entry_label)
    : cfg(cfg), label(std::move(entry_label)) {}

void BasicBlock::gen_asm(std::ostream &o) {
  for (auto &instruction : instrs) {
    instruction.genAsm(o);
  }
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t,
                             const std::vector<std::string> &params) {
  instrs.emplace_back(this, op, t, params);
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
  // TODO
}
void CFG::gen_asm_epilogue(std::ostream &o) {
  // TODO
}

void CFG::add_to_symbol_table(std::string name, Type t) {
  SymbolType[name] = t;
  SymbolIndex[name] = nextFreeSymbolIndex;
  nextFreeSymbolIndex += 4;
}
std::string CFG::create_new_tempvar(Type t) {
  std::string tempVarName = "!T" + std::to_string(nextFreeSymbolIndex);
  add_to_symbol_table(tempVarName, t);
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
