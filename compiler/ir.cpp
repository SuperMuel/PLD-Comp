#include "ir.h"
#include <string>

IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t,
                 const std::vector<std::string> &params)
    : block(bb_), op(op), outType(t), params(params) {}

void IRInstr::genAsm(std::ostream &os, CFG *cfg) {
  switch (op) {
  case add:
    cfg->freeRegister -= 2;
    os << "addl %" << registers[cfg->freeRegister + 1] << ", %"
       << registers[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case sub:
    cfg->freeRegister -= 2;
    os << "subl %" << registers[cfg->freeRegister + 1] << ", %"
       << registers[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case mul:
    cfg->freeRegister -= 2;
    os << "imull %" << registers[cfg->freeRegister + 1] << ", %"
       << registers[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case div:
    // Division behaves a little bit differently, it divides the contents of
    // edx:eax (where ':' means concatenation) with the content of the given
    // register The quotient is stored in eax and the remainder in edx
    os << "movl %" << registers[cfg->freeRegister - 2] << ", %eax" << std::endl;
    os << "movl $0, %edx" << std::endl;
    os << "idivl %" << registers[cfg->freeRegister - 1] << std::endl;
    cfg->freeRegister -= 2;
    os << "movl %eax, %" << registers[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case ret:
    os << "movl %" << registers[0] << ", %eax" << std::endl;
    os << "popq %rbp\n";
    os << "ret\n";
    break;
  case var_assign:
    os << "movl %" << registers[0] << ", -"
       << cfg->symbolTable[params[0]]->offset << "(%rbp)" << std::endl;
    cfg->freeRegister--;
    break;
  case ldconst:
    os << "movl $" << params[0] << ", %" << registers[cfg->freeRegister]
       << std::endl;
    cfg->freeRegister++;
    break;
  case ldvar:
    os << "movl -" << cfg->symbolTable[params[0]]->offset << "(%rbp), %"
       << registers[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case call:
    os << "call " << params[0] << "@PLT" << std::endl;
    break;
  case move: 
    os << "movl %" << registers[0] << ", %" << params[0] << std::endl;
    cfg->freeRegister--;
    break;
  }
}

std::ostream &operator<<(std::ostream &os, IRInstr &instruction) {
  switch (instruction.op) {
  case IRInstr::add:
    os << instruction.params[2] << " = " << instruction.params[0] << " + "
       << instruction.params[1];
    break;
  case IRInstr::sub:
    os << instruction.params[2] << " = " << instruction.params[0] << " - "
       << instruction.params[1];
    break;
  case IRInstr::div:
    os << instruction.params[2] << " = " << instruction.params[0] << " / "
       << instruction.params[1];
    break;
  case IRInstr::mul:
    os << instruction.params[2] << " = " << instruction.params[0] << " * "
       << instruction.params[1];
    break;
  case IRInstr::ldconst:
    os << instruction.params[1] << " = " << instruction.params[0];
    break;
  case IRInstr::ret:
    os << "ret " << instruction.params[0];
    break;
  case IRInstr::var_assign:
    os << instruction.params[0] << " = " << instruction.params[1];
    break;
  case IRInstr::ldvar:
    os << instruction.params[1] << " = " << instruction.params[0];
    break;
  case IRInstr::call:
    os << "call " << instruction.params[0];
    break;
  case IRInstr::move:
    os << "move " << instruction.params[0] << " to " << instruction.params[1];
    break;
  }
  return os;
}

BasicBlock::BasicBlock(CFG *cfg, std::string entry_label)
    : cfg(cfg), label(std::move(entry_label)) {}

void BasicBlock::gen_asm(std::ostream &o) {
  for (auto &instruction : instrs) {
    instruction.genAsm(o, cfg);
  }
}

std::string BasicBlock::add_IRInstr(IRInstr::Operation op, Type t,
                                    std::vector<std::string> params, CFG *cfg) {
  std::string dest;
  switch (op) {
  case IRInstr::add:
  case IRInstr::sub:
  case IRInstr::mul:
  case IRInstr::div:
  case IRInstr::ldvar:
  case IRInstr::ldconst:
  case IRInstr::call: {
    dest = cfg->create_new_tempvar(t);
    params.push_back(dest);
    instrs.emplace_back(this, op, t, params);
    break;
  }
  case IRInstr::ret:
  case IRInstr::var_assign:
  case IRInstr::move: {
    instrs.emplace_back(this, op, t, params);
    break;
  }
  }
  return dest;
}

CFG::~CFG() {
  for (auto bb : bbs) {
    delete bb;
  }
}

CFG::CFG() : nextFreeSymbolIndex(4), freeRegister(0) {}

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
  current_bb->gen_asm(o);
  gen_asm_epilogue(o);
}
void CFG::gen_asm_epilogue(std::ostream &o) {
  // TODO
}

void CFG::add_to_symbol_table(std::string name, Type t, int line) {
  Symbol *newSymbol = new Symbol(line);
  newSymbol->offset = nextFreeSymbolIndex;
  nextFreeSymbolIndex += 4;
  symbolTable[name] = newSymbol;
}

std::string CFG::create_new_tempvar(Type t) {
  std::string tempVarName = "!T" + std::to_string(nextFreeSymbolIndex);
  add_to_symbol_table(tempVarName, t, 0);
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
