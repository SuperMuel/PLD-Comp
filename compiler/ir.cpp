#include "ir.h"
#include "VisitorErrorListener.h"
#include <memory>
#include <string>

std::ostream &operator<<(std::ostream &os, const Parameter &param) {
  if (auto symbol = std::get_if<std::shared_ptr<Symbol>>(&param)) {
    os << (*symbol)->lexeme;
  } else if (auto n = std::get_if<std::string>(&param)) {
    os << *n;
  }

  return os;
}

IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t,
                 const std::vector<Parameter> &params)
    : block(bb_), op(op), outType(t), params(params) {}

void IRInstr::genAsm(std::ostream &os, CFG *cfg) {
  switch (op) {
  case add:
    cfg->freeRegister -= 2;
    os << "addl %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case sub:
    cfg->freeRegister -= 2;
    os << "subl %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case mul:
    cfg->freeRegister -= 2;
    os << "imull %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case cmpNZ:
    os << "testl %" << registers32[cfg->freeRegister - 1] << ", %"
       << registers32[cfg->freeRegister - 1] << std::endl;
    // os << "je " << params[1] << std::endl;
    cfg->freeRegister--;
    break;
  case div:
    // Division behaves a little bit differently, it divides the contents of
    // edx:eax (where ':' means concatenation) with the content of the given
    // register The quotient is stored in eax and the remainder in edx
    os << "movl %" << registers32[cfg->freeRegister - 2] << ", %eax"
       << std::endl;
    os << "movl $0, %edx" << std::endl;
    os << "idivl %" << registers32[cfg->freeRegister - 1] << std::endl;
    cfg->freeRegister -= 2;
    os << "movl %eax, %" << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case b_and:
    cfg->freeRegister -= 2;
    os << "andl %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case b_or:
    cfg->freeRegister -= 2;
    os << "orl %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case b_xor:
    cfg->freeRegister -= 2;
    os << "xorl %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case lt:
    cfg->freeRegister -= 2;
    os << "cmp %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    os << "setl %" << registers8[cfg->freeRegister] << std::endl;
    os << "movzbl %" << registers8[cfg->freeRegister] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case leq:
    cfg->freeRegister -= 2;
    os << "cmp %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    os << "setle %" << registers8[cfg->freeRegister] << std::endl;
    os << "movzbl %" << registers8[cfg->freeRegister] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case gt:
    cfg->freeRegister -= 2;
    os << "cmp %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    os << "setg %" << registers8[cfg->freeRegister] << std::endl;
    os << "movzbl %" << registers8[cfg->freeRegister] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case geq:
    cfg->freeRegister -= 2;
    os << "cmp %" << registers32[cfg->freeRegister + 1] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    os << "setge %" << registers8[cfg->freeRegister] << std::endl;
    os << "movzbl %" << registers8[cfg->freeRegister] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case eq:
    cfg->freeRegister -= 2;
    os << "cmp %" << registers32[cfg->freeRegister] << ", %"
       << registers32[cfg->freeRegister + 1] << std::endl;
    os << "sete %" << registers8[cfg->freeRegister] << std::endl;
    os << "movzbl %" << registers8[cfg->freeRegister] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case neq:
    cfg->freeRegister -= 2;
    os << "cmp %" << registers32[cfg->freeRegister] << ", %"
       << registers32[cfg->freeRegister + 1] << std::endl;
    os << "setne %" << registers8[cfg->freeRegister] << std::endl;
    os << "movzbl %" << registers8[cfg->freeRegister] << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case ret:
    os << "movl %" << registers32[cfg->freeRegister - 1] << ", %eax"
       << std::endl;
    os << "popq %rbp\n";
    os << "ret\n";
    cfg->freeRegister--;
    break;
  case var_assign:
    os << "movl %" << registers32[0] << ", -"
       << std::get<std::shared_ptr<Symbol>>(params[0])->offset << "(%rbp)"
       << std::endl;
    cfg->freeRegister--;
    break;
  case ldconst:
    os << "movl $" << std::get<std::string>(params[0]) << ", %"
       << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
    break;
  case ldvar:
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[0])->offset
       << "(%rbp), %" << registers32[cfg->freeRegister] << std::endl;
    cfg->freeRegister++;
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
  case IRInstr::lt:
    os << instruction.params[2] << " = " << instruction.params[0] << " < "
       << instruction.params[1];
    break;
  case IRInstr::leq:
    os << instruction.params[2] << " = " << instruction.params[0]
       << " <= " << instruction.params[1];
    break;
  case IRInstr::gt:
    os << instruction.params[2] << " = " << instruction.params[0] << " > "
       << instruction.params[1];
    break;
  case IRInstr::geq:
    os << instruction.params[2] << " = " << instruction.params[0]
       << " >= " << instruction.params[1];
    break;
  case IRInstr::eq:
    os << instruction.params[2] << " = " << instruction.params[0]
       << " == " << instruction.params[1];
    break;
  case IRInstr::neq:
    os << instruction.params[2] << " = " << instruction.params[0]
       << " != " << instruction.params[1];
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
  }
  return os;
}

BasicBlock::BasicBlock(CFG *cfg, std::string entry_label)
    : cfg(cfg), label(std::move(entry_label)), exit_true(nullptr),
      exit_false(nullptr), visited(false) {}

void BasicBlock::gen_asm(std::ostream &o) {
  if (visited) {
    return;
  }
  visited = true;
  if (!label.empty()) {
    std::cout << label << ":\n";
  }
  for (auto &instruction : instrs) {
    instruction.genAsm(o, cfg);
  }
  if (exit_false != nullptr) {
    o << "je " << exit_false->label << "\n";
  }
  if (exit_true != nullptr && !exit_true->label.empty()) {
    o << "jmp " << exit_true->label << "\n";
  }
  if (exit_true != nullptr) {
    exit_true->gen_asm(o);
  }
  if (exit_false != nullptr) {
    exit_false->gen_asm(o);
  }
}

std::shared_ptr<Symbol> BasicBlock::add_IRInstr(IRInstr::Operation op, Type t,
                                                std::vector<Parameter> params,
                                                CFG *cfg) {
  switch (op) {
  case IRInstr::add:
  case IRInstr::sub:
  case IRInstr::mul:
  case IRInstr::div:
  case IRInstr::b_and:
  case IRInstr::b_or:
  case IRInstr::b_xor:
  case IRInstr::lt:
  case IRInstr::leq:
  case IRInstr::gt:
  case IRInstr::geq:
  case IRInstr::eq:
  case IRInstr::neq:
  case IRInstr::ldvar:
  case IRInstr::cmpNZ:
  case IRInstr::ldconst: {
    std::shared_ptr<Symbol> symbol = cfg->create_new_tempvar(t);
    params.push_back(symbol);
    instrs.emplace_back(this, op, t, params);
    return symbol;
    break;
  }
  case IRInstr::ret:
  case IRInstr::var_assign: {
    instrs.emplace_back(this, op, t, params);
    break;
  }
  }
  return nullptr;
}

CFG::~CFG() {
  while (!symbolTables.empty()) {
    pop_table();
  }
  for (auto bb : bbs) {
    delete bb;
  }
}

CFG::CFG() : nextFreeSymbolIndex(4), freeRegister(0) { push_table(); }

void CFG::add_bb(BasicBlock *bb) {
  bbs.push_back(bb);
  current_bb = bb;
}

std::string CFG::IR_reg_to_asm(std::string reg) {
  // TODO
  return "";
}

void CFG::gen_asm_prologue(std::ostream &o) {
#ifdef __APPLE__
  o << ".globl _main\n";
  o << " _main: \n";
#else
  o << ".globl main\n";
  o << "main: \n";
#endif
  o << "pushq %rbp\n";
  o << "movq %rsp, %rbp\n";
}

void CFG::gen_asm(std::ostream &o) {
  gen_asm_prologue(o);
  bbs[0]->gen_asm(o);
  gen_asm_epilogue(o);
}

void CFG::gen_asm_epilogue(std::ostream &o) {
  // TODO
}

void CFG::pop_table() {
  for (auto it = symbolTables.front().begin(); it != symbolTables.front().end();
       it++) {
    if (!it->second->used) {
      VisitorErrorListener::addError("Variable " + it->first +
                                         " not used (declared in line " +
                                         std::to_string(it->second->line) + ")",
                                     ErrorType::Warning);
    }
  }
  symbolTables.pop_front();
}

bool CFG::add_symbol(std::string id, Type t, int line) {
  if (symbolTables.front().count(id)) {
    return false;
  }
  std::shared_ptr<Symbol> newSymbol = std::make_shared<Symbol>(id, line);
  newSymbol->offset = nextFreeSymbolIndex;
  nextFreeSymbolIndex += 4;
  symbolTables.front()[id] = newSymbol;

  return true;
}

std::shared_ptr<Symbol> CFG::get_symbol(const std::string &name) {
  auto it = symbolTables.begin();
  while (it != symbolTables.end()) {
    auto symbol = it->find(name);
    if (symbol != it->end()) {
      return symbol->second;
    }
    it++;
  }
  return nullptr;
}

std::shared_ptr<Symbol> CFG::create_new_tempvar(Type t) {
  std::string tempVarName = "!T" + std::to_string(nextFreeSymbolIndex);
  if (add_symbol(tempVarName, t, 0)) {
    std::shared_ptr<Symbol> symbol = get_symbol(tempVarName);
    symbol->used = true;
    return symbol;
  }
  return nullptr;
}
int CFG::get_var_index(std::string name) {
  // TODO
  return 0;
}
Type CFG::get_var_type(std::string name) {
  // TODO
  return Type::INT;
}
