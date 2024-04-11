#include "ir.h"
#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
#include <iostream>
#include <memory>
#include <queue>
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
    handleBinaryOp("addl", os, cfg);
    break;
  case sub:
    handleBinaryOp("subl", os, cfg);
    break;
  case mul:
    handleBinaryOp("imull", os, cfg);
    break;
  case cmpNZ:
    handleCmpNZ(os, cfg);
    break;
  case div:
    handleDiv(os, cfg);
    break;
  case mod:
    handleMod(os, cfg);
    break;
  case b_and:
    handleBinaryOp("andl", os, cfg);
    break;
  case b_or:
    handleBinaryOp("orl", os, cfg);
    break;
  case b_xor:
    handleBinaryOp("xorl", os, cfg);
    break;
  case lt:
    handleCmpOp("setl", os, cfg);
    break;
  case leq:
    handleCmpOp("setle", os, cfg);
    break;
  case gt:
    handleCmpOp("setg", os, cfg);
    break;
  case geq:
    handleCmpOp("setge", os, cfg);
    break;
  case eq:
    handleCmpOp("sete", os, cfg);
    break;
  case neq:
    handleCmpOp("setne", os, cfg);
    break;
  case ret:
    handleRet(os, cfg);
    break;
  case var_assign:
    handleVar_assign(os, cfg);
    break;
  case ldconst:
    handleLdconst(os, cfg);
    break;
  case ldvar:
    handleLdvar(os, cfg);
  case neg:
    handleUnaryOp("neg", os, cfg);
    break;
  case not_:
    handleUnaryOp("notl", os, cfg);
    break;
  case lnot:
    handleUnaryOp("lnot", os, cfg);
    break;
  case inc:
    handleUnaryOp("inc", os, cfg);
    break;
  case dec:
    handleUnaryOp("dec", os, cfg);
    break;
  case nothing:
    break;
  case call:
    handleCall(os, cfg);
    break;
  case param:
    handleParam(os, cfg);
    break;
  case param_decl:
    break;
  }
}

std::set<std::shared_ptr<Symbol>> IRInstr::getUsedVariables() {
  std::set<std::shared_ptr<Symbol>> result;
  switch (op) {
  case IRInstr::add:
  case IRInstr::sub:
  case IRInstr::mul:
  case IRInstr::div:
  case IRInstr::mod:
  case IRInstr::b_and:
  case IRInstr::b_or:
  case IRInstr::b_xor:
  case IRInstr::lt:
  case IRInstr::leq:
  case IRInstr::gt:
  case IRInstr::geq:
  case IRInstr::eq:
  case IRInstr::neq:
    result.insert(std::get<std::shared_ptr<Symbol>>(params[0]));
    result.insert(std::get<std::shared_ptr<Symbol>>(params[1]));
    break;
  case IRInstr::ldconst:
    break;
  case IRInstr::var_assign:
  case IRInstr::lnot:
    result.insert(std::get<std::shared_ptr<Symbol>>(params[1]));
    break;
  case IRInstr::cmpNZ:
  case IRInstr::neg:
  case IRInstr::not_:
  case IRInstr::inc:
  case IRInstr::dec:
  case IRInstr::param:
    result.insert(std::get<std::shared_ptr<Symbol>>(params[0]));
    break;
  case ret:
    if (outType != Type::VOID) {
      result.insert(std::get<std::shared_ptr<Symbol>>(params[0]));
    }
    break;
  case IRInstr::nothing:
  case IRInstr::ldvar:
  case IRInstr::param_decl:
    break;
  case IRInstr::call: {
    int cnt = params.size();
    if (outType != Type::VOID) {
      cnt--;
    }
    for (int i = 1; i < cnt; i++) {
      result.insert(std::get<std::shared_ptr<Symbol>>(params[i]));
    }
    break;
  }
  }
  return result;
}

std::set<std::shared_ptr<Symbol>> IRInstr::getDeclaredVariable() {
  std::set<std::shared_ptr<Symbol>> result;
  switch (op) {
  case IRInstr::add:
  case IRInstr::sub:
  case IRInstr::mul:
  case IRInstr::div:
  case IRInstr::mod:
  case IRInstr::b_and:
  case IRInstr::b_or:
  case IRInstr::b_xor:
  case IRInstr::lt:
  case IRInstr::leq:
  case IRInstr::gt:
  case IRInstr::geq:
  case IRInstr::eq:
  case IRInstr::neq:
    result.insert(std::get<std::shared_ptr<Symbol>>(params[2]));
    break;
  case IRInstr::ldconst:
  case IRInstr::lnot:
    result.insert(std::get<std::shared_ptr<Symbol>>(params[1]));
    break;
  case IRInstr::var_assign:
  case IRInstr::neg:
  case IRInstr::not_:
  case IRInstr::inc:
  case IRInstr::dec:
  case IRInstr::param_decl:
    result.insert(std::get<std::shared_ptr<Symbol>>(params[0]));
    break;
  case IRInstr::call:
    if (outType != Type::VOID) {
      result.insert(
          std::get<std::shared_ptr<Symbol>>(params[params.size() - 1]));
    }
    break;
  case IRInstr::ret:
  case IRInstr::cmpNZ:
  case IRInstr::ldvar:
  case IRInstr::nothing:
  case IRInstr::param:
    break;
  }
  return result;
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
  case IRInstr::mod:
    os << instruction.params[2] << " = " << instruction.params[0] << " % "
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
  case IRInstr::b_and:
    os << instruction.params[2] << " = " << instruction.params[0] << " & "
       << instruction.params[1];
    break;
  case IRInstr::b_or:
    os << instruction.params[2] << " = " << instruction.params[0] << " | "
       << instruction.params[1];
    break;
  case IRInstr::b_xor:
    os << instruction.params[2] << " = " << instruction.params[0] << " ^ "
       << instruction.params[1];
    break;
  case IRInstr::ldconst:
    os << instruction.params[1] << " = " << instruction.params[0];
    break;
  case IRInstr::ldvar:
    os << "ldvar " << instruction.params[0];
    break;
  case IRInstr::ret:
    os << "ret " << instruction.params[0];
    break;
  case IRInstr::var_assign:
    os << instruction.params[0] << " = " << instruction.params[1];
    break;
  case IRInstr::cmpNZ:
    os << instruction.params[0] << " !=  0";
    break;
  case IRInstr::neg:
    os << " - " << instruction.params[0];
    break;
  case IRInstr::not_:
    os << " ~ " << instruction.params[0];
    break;
  case IRInstr::lnot:
    os << instruction.params[1] << "= ! " << instruction.params[0];
    break;
  case IRInstr::inc:
    os << "++" << instruction.params[0];
    break;
  case IRInstr::dec:
    os << "--" << instruction.params[0];
    break;
  case IRInstr::nothing:
  case IRInstr::call:
    if (instruction.params.size() == 2) {
      os << instruction.params[1] << " = call " << instruction.params[0];
    } else {
      os << "call " << instruction.params[0];
    }
    break;
  case IRInstr::param:
    os << "param " << instruction.params[0];
    break;
  case IRInstr::param_decl:
    os << "param_decl " << instruction.params[0];
    break;
  }
  return os;
}

void IRInstr::handleCmpNZ(std::ostream &os, CFG *cfg) {

  int firstRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[0]));
  if (firstRegister == cfg->scratchRegister) {
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[0])->offset
       << "(%rbp), " << registers32[firstRegister] << std::endl;
  }

  os << "testl %" << registers32[firstRegister] << ", %"
     << registers32[firstRegister] << std::endl;
}

void IRInstr::handleDiv(std::ostream &os, CFG *cfg) {
  // Division behaves a little bit differently, it divides the contents of
  // edx:eax (where ':' means concatenation) with the content of the given
  // register The quotient is stored in eax and the remainder in edx

  int firstRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[0]));
  int secondRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[1]));
  int destRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[2]));
  if (firstRegister == cfg->scratchRegister) {
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[0])->offset
       << "(%rbp), %" << registers32[firstRegister] << std::endl;
  }
  os << "movl %" << registers32[firstRegister] << ", %eax" << std::endl;
  os << "movl $0, %edx" << std::endl;
  if (secondRegister == cfg->scratchRegister) {
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[1])->offset
       << "(%rbp), " << registers32[secondRegister] << std::endl;
  }
  os << "idivl %" << registers32[secondRegister] << std::endl;
  os << "movl %eax, %" << registers32[destRegister] << std::endl;
  if (destRegister == cfg->scratchRegister) {
    os << "movl %" << registers32[destRegister] << ", -"
       << std::get<std::shared_ptr<Symbol>>(params[2])->offset << "(%rbp)"
       << std::endl;
  }
}

void IRInstr::handleMod(std::ostream &os, CFG *cfg) {
  int firstRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[0]));
  int secondRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[1]));
  int destRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[2]));
  if (firstRegister == cfg->scratchRegister) {
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[0])->offset
       << "(%rbp), %" << registers32[firstRegister] << std::endl;
  }
  os << "movl %" << registers32[firstRegister] << ", %eax" << std::endl;
  os << "movl $0, %edx" << std::endl;

  if (secondRegister == cfg->scratchRegister) {
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[1])->offset
       << "(%rbp), %" << registers32[secondRegister] << std::endl;
  }
  os << "idivl %" << registers32[secondRegister] << std::endl;
  os << "movl %edx, %" << registers32[destRegister] << std::endl;
  if (destRegister == cfg->scratchRegister) {
    os << "movl %" << registers32[destRegister] << ",-"
       << std::get<std::shared_ptr<Symbol>>(params[2])->offset << "(%rbp)"
       << std::endl;
  }
}

void IRInstr::handleRet(std::ostream &os, CFG *cfg) {
  if (outType != Type::VOID) {
    int firstRegister =
        cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[0]));

    if (firstRegister == cfg->scratchRegister) {
      os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[0])->offset
         << "(%rbp), %" << registers32[firstRegister] << std::endl;
    }
    os << "movl %" << registers32[firstRegister] << ", %eax" << std::endl;
  }
  os << "popq %rbp\n";
  os << "ret\n";
}

void IRInstr::handleVar_assign(std::ostream &os, CFG *cfg) {
  int destRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[0]));
  int sourceRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[1]));
  auto symbol = std::get<std::shared_ptr<Symbol>>(params[0]);
  std::string instr = (symbol->type == Type::CHAR ? "movb " : "movl ");
  const std::string *registers =
      (symbol->type == Type::CHAR ? registers8 : registers32);

  if (sourceRegister == cfg->scratchRegister) {
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[1])->offset
       << "(%rbp), %" << registers32[sourceRegister] << std::endl;
  }

  if (sourceRegister != destRegister) {
    os << "movl %" << registers32[sourceRegister] << ", %"
       << registers32[destRegister] << "\n";
  }
  if (destRegister == cfg->scratchRegister) {
    os << instr << " %" << registers32[destRegister] << ", -" << symbol->offset
       << "(%rbp)" << std::endl;
  }
}

void IRInstr::handleLdconst(std::ostream &os, CFG *cfg) {
  auto symbol = std::get<std::shared_ptr<Symbol>>(params[1]);
  auto val = std::get<std::string>(params[0]);
  int destRegister = cfg->findRegister(symbol);
  std::string instr = (symbol->type == Type::CHAR ? "movb" : "movl");
  const std::string *registers =
      (symbol->type == Type::CHAR ? registers8 : registers32);

  os << "movl $" << val << ", %" << registers32[destRegister] << std::endl;
  if (destRegister == cfg->scratchRegister) {
    os << "movl %" << registers32[destRegister] << ", -"
       << std::get<std::shared_ptr<Symbol>>(params[1])->offset << "(%rbp)"
       << std::endl;
  }
}

void IRInstr::handleLdvar(std::ostream &os, CFG *cfg) {
  // auto symbol = std::get<std::shared_ptr<Symbol>>(params[0]);
  // std::string instr = (symbol->type == Type::CHAR ? "movsbl" : "movl");

  /**os << instr << " -" << symbol->offset << "(%rbp), %"
     << "rax" << std::endl;*/
}

void IRInstr::handleBinaryOp(const std::string &op, std::ostream &os,
                             CFG *cfg) {

  int firstRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[0]));
  int secondRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[1]));
  int destRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[2]));
  if (firstRegister == cfg->scratchRegister) {
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[0])->offset
       << "(%rbp), %" << registers32[firstRegister] << std::endl;
  }
  if (firstRegister == cfg->scratchRegister &&
      secondRegister == cfg->scratchRegister &&
      destRegister == cfg->scratchRegister) {
    os << op << " -" << std::get<std::shared_ptr<Symbol>>(params[1])->offset
       << "(%rbp), %" << registers32[destRegister] << std::endl;
  } else if (destRegister != secondRegister) {
    if (destRegister != firstRegister) {
      os << "movl %" << registers32[firstRegister] << ", %"
         << registers32[destRegister] << "\n";
    }
    if (secondRegister == cfg->scratchRegister) {
      os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[1])->offset
         << "(%rbp), %" << registers32[secondRegister] << std::endl;
    }
    os << op << " %" << registers32[secondRegister] << ", %"
       << registers32[destRegister] << std::endl;
  } else {
    if (secondRegister != cfg->scratchRegister) {
      if (firstRegister != cfg->scratchRegister) {
        os << "movl %" << registers32[secondRegister] << ", %"
           << registers32[cfg->scratchRegister] << "\n";
        os << "movl %" << registers32[firstRegister] << ", %"
           << registers32[destRegister] << "\n";
        os << op << " %" << registers32[cfg->scratchRegister] << ", %"
           << registers32[destRegister] << std::endl;
      } else {
        os << "xchg %" << registers32[secondRegister] << ", %"
           << registers32[firstRegister] << std::endl;
        os << op << " %" << registers32[firstRegister] << ", %"
           << registers32[destRegister] << std::endl;
      }
    } else {
      os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[1])->offset
         << "(%rbp), %" << registers32[secondRegister] << std::endl;
      if (destRegister != firstRegister) {
        os << "movl %" << registers32[firstRegister] << ", %"
           << registers32[destRegister] << "\n";
      }
      os << op << " %" << registers32[secondRegister] << ", %"
         << registers32[destRegister] << std::endl;
    }
  }
}

void IRInstr::handleCmpOp(const std::string &op, std::ostream &os, CFG *cfg) {
  int firstRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[0]));
  int secondRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[1]));
  int destRegister =
      cfg->findRegister(std::get<std::shared_ptr<Symbol>>(params[2]));
  if (firstRegister == cfg->scratchRegister &&
      secondRegister == cfg->scratchRegister) {
    os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[0])->offset
       << "(%rbp), %" << registers32[firstRegister] << std::endl;
    os << "cmp -" << std::get<std::shared_ptr<Symbol>>(params[1])->offset
       << "(%rbp)"
       << ", %" << registers32[firstRegister] << std::endl;
    os << "movzbl %" << registers8[cfg->scratchRegister] << ", %"
       << registers32[destRegister] << std::endl;
  } else {
    if (firstRegister == cfg->scratchRegister) {
      os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[0])->offset
         << "(%rbp), %" << registers32[firstRegister] << std::endl;
    } else if (secondRegister == cfg->scratchRegister) {
      os << "movl -" << std::get<std::shared_ptr<Symbol>>(params[1])->offset
         << "(%rbp), %" << registers32[secondRegister] << std::endl;
    }
    os << "cmp %" << registers32[secondRegister] << ", %"
       << registers32[firstRegister] << std::endl;
    os << op << " %" << registers8[cfg->scratchRegister] << std::endl;
    os << "movzbl %" << registers8[cfg->scratchRegister] << ", %"
       << registers32[destRegister] << std::endl;
  }

  if (destRegister == cfg->scratchRegister) {
    os << "movl %" << registers32[destRegister] << ", -"
       << std::get<std::shared_ptr<Symbol>>(params[2])->offset << "(%rbp)";
  }
}

int CFG::findRegister(std::shared_ptr<Symbol> &param) {
  auto paramLocation = registerAssignment.find(param);
  if (paramLocation != registerAssignment.end()) {
    return paramLocation->second;
  }
  return scratchRegister;
}

void IRInstr::handleUnaryOp(const std::string &op, std::ostream &os, CFG *cfg) {
  auto symbol = std::get<std::shared_ptr<Symbol>>(params[0]);
  int varRegister = cfg->findRegister(symbol);

  if (op == "inc" || op == "dec") {
    if (varRegister == cfg->scratchRegister) {
      os << "movl -" << symbol->offset << "(%rbp), %"
         << registers32[varRegister] << std::endl;
    }
    os << op << " %" << registers32[varRegister] << "\n";
    if (varRegister == cfg->scratchRegister) {
      os << "movl %" << registers32[varRegister] << ", -" << symbol->offset
         << "(%rbp)" << std::endl;
    }
  }
  if (op == "neg" || op == "notl") {
    if (varRegister == cfg->scratchRegister) {
      os << "movl -" << symbol->offset << "(%rbp), %"
         << registers32[varRegister] << std::endl;
    } else {
      os << "movl %" << registers32[varRegister] << ", %"
         << registers32[cfg->scratchRegister] << std::endl;
    }
    os << op << " %" << registers32[cfg->scratchRegister] << "\n";
    auto destSymbol = std::get<std::shared_ptr<Symbol>>(params[1]);
    int destRegister = cfg->findRegister(destSymbol);
    if (destRegister == cfg->scratchRegister) {
      os << "movl " << registers32[cfg->scratchRegister] << ", -"
         << destSymbol->offset << "(%rbp)" << std::endl;
    } else {
      os << "movl %" << registers32[cfg->scratchRegister] << ", %"
         << registers32[destRegister] << std::endl;
    }
  } else if (op == "lnot") {
    os << "cmpl $0, %" << registers32[varRegister] << std::endl;
    int varRegister = cfg->findRegister(symbol);
    if (varRegister == cfg->scratchRegister) {
      os << "movl %" << registers32[varRegister] << ", -" << symbol->offset
         << "(%rbp)"
         << "\n";
    }
    auto destSymbol = std::get<std::shared_ptr<Symbol>>(params[1]);
    int destRegister = cfg->findRegister(destSymbol);
    if (destRegister == cfg->scratchRegister) {
      os << "movl -" << symbol->offset << "(%rbp), %"
         << registers32[destRegister] << std::endl;
    }
    os << "sete %" << registers8[destRegister] << std::endl;
    os << "movzbl %" << registers8[destRegister] << ", %"
       << registers32[destRegister] << std::endl;
    if (destRegister == cfg->scratchRegister) {
      os << "movl %" << registers32[destRegister] << ", -" << symbol->offset
         << "(%rbp)" << std::endl;
    }
  }
}

void IRInstr::handleCall(std::ostream &os, CFG *cfg) {
  std::string funcName = std::get<std::string>(params[0]);
  CFG *function = cfg->get_visitor()->getFunction(funcName);
  int paramNum = function->get_parameters_type().size();

  int val = (cfg->nextFreeSymbolIndex + 16 - 1) / 16 * 16;
  if (val) {
    os << "subq $" << val << ", %rsp" << std::endl;
  }
  for (int i = 0; i < 8; i++) {
    os << "pushq %" << registers64[i] << std::endl;
  }

  bool exchange = false;
  if (paramNum >= 6) {
    auto symbol8 = std::get<std::shared_ptr<Symbol>>(params[5]);
    auto symbol9 = std::get<std::shared_ptr<Symbol>>(params[6]);
    if (cfg->findRegister(symbol8) == 1 && cfg->findRegister(symbol9) == 0) {
      os << "xchg %r8d, %r9d" << std::endl;
      exchange = true;
    }
  }
  for (int i = paramNum - 1; i >= 0; i--) {
    if ((i == 4 || i == 5) && exchange) {
      continue;
    }
    auto symbol = std::get<std::shared_ptr<Symbol>>(params[i + 1]);
    int paramRegister = cfg->findRegister(symbol);
    if (paramRegister == cfg->scratchRegister) {
      os << "movl -" << symbol->offset << "(%rbp), %"
         << registers32[paramRegister] << std::endl;
    }
    if (i < 6) {
      os << "movl %" << registers32[paramRegister] << ", %" << paramRegisters[i]
         << std::endl;
    } else {
      os << "pushq %" << registers64[paramRegister] << std::endl;
    }
  }

  auto returnVar = std::get<std::shared_ptr<Symbol>>(*params.rbegin());

  os << "call " << funcName << std::endl;
  int paramRegister = cfg->findRegister(returnVar);

  for (int i = 0; i < 8; i++) {
    os << "popq %" << registers64[7 - i] << std::endl;
  }

  for (int i = 7; i <= paramNum; i++) {
    os << "popq %" << registers64[cfg->scratchRegister] << std::endl;
  }
  if (val) {
    os << "addq $" << val << ", %rsp" << std::endl;
  }

  if (outType != Type::VOID) {
    os << "movl %eax, %" << registers32[paramRegister] << std::endl;
    os << "movl %" << registers32[paramRegister] << ", -" << returnVar->offset
       << "(%rbp)" << std::endl;
  }

  if (outType != Type::VOID && paramRegister != cfg->scratchRegister) {
    os << "movl -" << returnVar->offset << "(%rbp)"
       << ", %" << registers32[paramRegister] << std::endl;
  }
}

void IRInstr::handleParam(std::ostream &os, CFG *cfg) {
  cfg->push_parameter(std::get<std::shared_ptr<Symbol>>(params[0]));
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
                                                std::vector<Parameter> params) {
  switch (op) {
  case IRInstr::add:
  case IRInstr::sub:
  case IRInstr::mul:
  case IRInstr::div:
  case IRInstr::mod:
  case IRInstr::b_and:
  case IRInstr::b_or:
  case IRInstr::b_xor:
  case IRInstr::lt:
  case IRInstr::leq:
  case IRInstr::gt:
  case IRInstr::geq:
  case IRInstr::eq:
  case IRInstr::neq:
  case IRInstr::cmpNZ:
  case IRInstr::neg:
  case IRInstr::not_:
  case IRInstr::ldconst:
  case IRInstr::lnot: {
    std::shared_ptr<Symbol> symbol = cfg->create_new_tempvar(t);
    params.push_back(symbol);
    instrs.emplace_back(this, op, t, params);
    return symbol;
    break;
  }
  case IRInstr::call: {
    if (t != Type::VOID) {
      std::shared_ptr<Symbol> symbol = cfg->create_new_tempvar(t);
      params.push_back(symbol);
      instrs.emplace_back(this, op, t, params);
      return symbol;
    } else {
      instrs.emplace_back(this, op, t, params);
    }
    break;
  }
  case IRInstr::ret:
  case IRInstr::param:
  case IRInstr::var_assign: {
    instrs.emplace_back(this, op, t, params);
    break;
  }
  case IRInstr::inc:
  case IRInstr::dec:
    instrs.emplace_back(this, op, t, params);
    return std::get<std::shared_ptr<Symbol>>(params[0]);
    break;

  case IRInstr::param_decl:
    instrs.emplace_back(this, op, t, params);
    return std::get<std::shared_ptr<Symbol>>(params[0]);
  case IRInstr::ldvar:
    return std::get<std::shared_ptr<Symbol>>(params[0]);
    break;
  case IRInstr::nothing:
    break;
  }

  return nullptr;
}

CFG::CFG(Type type, const std::string &name, int argCount,
         CodeGenVisitor *visitor)
    : nextFreeSymbolIndex(1 + 4 * std::max(0, argCount - 6)), name(name),
      returnType(type), visitor(visitor) {
  add_bb(new BasicBlock(this, ""));
  push_table();
}

CFG::~CFG() {
  while (!symbolTables.empty()) {
    pop_table();
  }
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
#ifdef __APPLE__
  o << ".globl _" << name << "\n";
  o << "_" << name << " : \n";
#else
  o << ".globl " << name << "\n";
  o << name << " : \n";
#endif
  o << "pushq %rbp\n";
  o << "movq %rsp, %rbp\n";

  bool isSwapped = false;
  if (parameterTypes.size() >= 6) {
    int parameterRegister1 = findRegister(parameterTypes[4].symbol);
    int parameterRegister2 = findRegister(parameterTypes[4].symbol);
    if (parameterRegister1 == 5 && parameterRegister2 == 4) {
      o << "xchg %r8d, %r9d" << std::endl;
    }
  }

  for (int i = 4; !isSwapped && i < std::min((int)parameterTypes.size(), 6);
       i++) {
    auto parameter = parameterTypes[i];
    int parameterRegister = findRegister(parameter.symbol);
    o << "movl %" << paramRegisters[i] << ", %"
      << registers32[parameterRegister] << std::endl;
    if (parameterRegister == scratchRegister) {
      o << "movl %" << registers32[parameterRegister] << ", -"
        << parameter.symbol->offset << "(%rbp)" << std::endl;
    }
  }
  for (int i = 0; i < parameterTypes.size(); i++) {
    if (i == 4 || i == 5) {
      continue;
    }
    auto parameter = parameterTypes[i];
    int parameterRegister = findRegister(parameter.symbol);
    if (i < 6) {
      o << "movl %" << paramRegisters[i] << ", %"
        << registers32[parameterRegister] << std::endl;
    } else {
      o << "movl " << 8 * (i - 4) << "(%rbp)"
        << ", %" << registers32[parameterRegister] << std::endl;
    }
    if (parameterRegister == scratchRegister) {
      o << "movl %" << registers32[parameterRegister] << ", -"
        << parameter.symbol->offset << "(%rbp)" << std::endl;
    }
  }
}

void CFG::gen_asm(std::ostream &o) {
  computeRegisterAllocation();
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
  std::shared_ptr<Symbol> newSymbol = std::make_shared<Symbol>(t, id, line);
  unsigned int sz = getSize(t);
  // This expression handles stack alignment
  newSymbol->offset = (nextFreeSymbolIndex + 2 * (sz - 1)) / sz * sz;
  nextFreeSymbolIndex += sz;
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
  unsigned int sz = getSize(t);
  unsigned int offset = (nextFreeSymbolIndex + 2 * (sz - 1)) / sz * sz;
  std::string tempVarName = "!T" + std::to_string(offset);
  if (add_symbol(tempVarName, t, 0)) {
    std::shared_ptr<Symbol> symbol = get_symbol(tempVarName);
    symbol->used = true;
    return symbol;
  }
  return nullptr;
}

std::shared_ptr<Symbol> CFG::add_parameter(const std::string &name, Type type,
                                           int line) {
  bool new_symbol = add_symbol(name, type, line);
  if (!new_symbol) {
    VisitorErrorListener::addError(
        "A parameter with name " + name + " has already been declared", line);
  }
  auto symbol = get_symbol(name);
  parameterTypes.emplace_back(type, symbol);
  return symbol;
}

int CFG::get_var_index(std::string name) {
  // TODO
  return 0;
}
Type CFG::get_var_type(std::string name) {
  // TODO
  return Type::INT;
}

LivenessInfo CFG::computeLiveInfo() {
  LivenessInfo liveInfo;
  bool stable = false;
  while (!stable) {
    stable = true;
    std::set<BasicBlock *> visitedBB;
    std::stack<BasicBlock *> visitOrder;
    visitOrder.push(bbs[0]);
    while (!visitOrder.empty()) {
      BasicBlock *currentBB = visitOrder.top();
      visitOrder.pop();
      visitedBB.insert(currentBB);
      int instructionIndex = 0;
      for (auto &instr : currentBB->instrs) {
        std::set<std::shared_ptr<Symbol>> oldIn;
        std::set<std::shared_ptr<Symbol>> oldOut;
        auto inPtr = liveInfo.liveIn.find(&instr);
        auto outPtr = liveInfo.liveOut.find(&instr);
        if (inPtr != liveInfo.liveIn.end()) {
          oldIn = inPtr->second;
        }
        if (outPtr != liveInfo.liveOut.end()) {
          oldOut = outPtr->second;
        }
        std::set<std::shared_ptr<Symbol>> inUnion = oldOut;
        liveInfo.liveIn[&instr] = instr.getUsedVariables();
        for (auto &toRemove : instr.getDeclaredVariable()) {
          inUnion.erase(toRemove);
        }
        for (auto &toAdd : inUnion) {
          liveInfo.liveIn[&instr].insert(toAdd);
        }
        liveInfo.liveOut[&instr].clear();
        std::vector<IRInstr *> nextInstrs;
        if (instructionIndex + 1 < currentBB->instrs.size()) {
          nextInstrs.push_back(&currentBB->instrs[instructionIndex + 1]);
        } else {
          // Perform a bfs on the next possible instruction
          std::set<BasicBlock *> bfsBB;
          std::queue<BasicBlock *> visitOrder;
          if (currentBB->exit_true != nullptr) {
            visitOrder.push(currentBB->exit_true);
          }
          if (currentBB->exit_false != nullptr) {
            visitOrder.push(currentBB->exit_false);
          }
          while (!visitOrder.empty()) {
            BasicBlock *currentVisitedBB = visitOrder.front();
            visitOrder.pop();
            bfsBB.insert(currentVisitedBB);
            if (currentVisitedBB->instrs.size() > 0) {
              nextInstrs.push_back(&currentVisitedBB->instrs[0]);
            } else {
              if (currentVisitedBB->exit_true != nullptr) {
                visitOrder.push(currentVisitedBB->exit_true);
              }
              if (currentVisitedBB->exit_false != nullptr) {
                visitOrder.push(currentVisitedBB->exit_false);
              }
            }
          }
        }
        for (auto &nextInstruction : nextInstrs) {
          for (auto var : liveInfo.liveIn[nextInstruction]) {
            liveInfo.liveOut[&instr].insert(var);
          }
        }
        if (stable && (liveInfo.liveIn[&instr] != oldIn ||
                       liveInfo.liveOut[&instr] != oldOut)) {
          stable = false;
        }
        instructionIndex++;
      }
      if (currentBB->exit_true != nullptr &&
          visitedBB.find(currentBB->exit_true) == visitedBB.end()) {
        visitOrder.push(currentBB->exit_true);
      }
      if (currentBB->exit_false != nullptr &&
          visitedBB.find(currentBB->exit_false) == visitedBB.end()) {
        visitOrder.push(currentBB->exit_false);
      }
    }
  }
  return liveInfo;
}

int computeNeighbors(std::vector<std::shared_ptr<Symbol>> &neighbors,
                     std::set<std::shared_ptr<Symbol>> &usedNodes) {
  int neighborCount = 0;
  for (auto x : neighbors) {
    if (std::find(usedNodes.begin(), usedNodes.end(), x) == usedNodes.end()) {
      neighborCount++;
    }
  }
  return neighborCount;
}

spillInformation CFG::findColorOrder(
    std::map<std::shared_ptr<Symbol>, std::vector<std::shared_ptr<Symbol>>>
        &interferenceGraph,
    int registerCount) {
  int n = interferenceGraph.size();
  spillInformation spillInfo;
  std::set<std::shared_ptr<Symbol>> usedNodes;
  int unselectedNodes = 0;
  while (unselectedNodes < n) {
    bool foundNode = false;
    for (auto node = interferenceGraph.begin(); node != interferenceGraph.end();
         node++) {
      if (usedNodes.find(node->first) == usedNodes.end() &&
          computeNeighbors(node->second, usedNodes) < registerCount) {
        spillInfo.colorOrder.push(node->first);
        usedNodes.insert(node->first);
        foundNode = true;
        break;
      }
    }
    if (!foundNode) {
      // Just spill the first variable
      for (auto node = interferenceGraph.begin();
           node != interferenceGraph.end(); node++) {
        if (std::find(usedNodes.begin(), usedNodes.end(), node->first) ==
            usedNodes.end()) {
          usedNodes.insert(node->first);
          spillInfo.spilledVariables.insert(node->first);
          break;
        }
      }
    }
    unselectedNodes++;
  }
  return spillInfo;
}

std::map<std::shared_ptr<Symbol>, int> CFG::assignRegisters(
    spillInformation &spillInfo,
    std::map<std::shared_ptr<Symbol>, std::vector<std::shared_ptr<Symbol>>>
        &interferenceGraph,
    int registerCount) {
  int n = interferenceGraph.size();
  std::map<std::shared_ptr<Symbol>, int> color;
  while (!spillInfo.colorOrder.empty()) {
    auto currentNode = spillInfo.colorOrder.top();
    spillInfo.colorOrder.pop();
    for (int curColor = 0; curColor < registerCount; curColor++) {
      bool colorAvailable = true;
      for (auto x : interferenceGraph[currentNode]) {
        if (color.find(x) != color.end() && color[x] == curColor) {
          colorAvailable = false;
          break;
        }
      }
      if (colorAvailable) {
        color[currentNode] = curColor;
        break;
      }
    }
  }
  return color;
}

std::map<std::shared_ptr<Symbol>, std::vector<std::shared_ptr<Symbol>>>
CFG::buildInterferenceGraph(LivenessInfo &liveInfo) {
  std::map<std::shared_ptr<Symbol>, std::vector<std::shared_ptr<Symbol>>>
      interferenceGraph;
  for (auto inPtr : liveInfo.liveIn) {
    auto declaredVar = inPtr.first->getDeclaredVariable();
    if (!declaredVar.empty()) {
      auto definedVariable = *declaredVar.begin();
      if (interferenceGraph.find(definedVariable) == interferenceGraph.end()) {
        interferenceGraph[definedVariable] =
            std::vector<std::shared_ptr<Symbol>>();
      }
      for (auto &outVar : liveInfo.liveOut[inPtr.first]) {
        if (outVar != definedVariable &&
            std::find(interferenceGraph[definedVariable].begin(),
                      interferenceGraph[definedVariable].end(),
                      outVar) == interferenceGraph[definedVariable].end()) {
          interferenceGraph[definedVariable].push_back(outVar);
          interferenceGraph[outVar].push_back(definedVariable);
        }
      }
    }
  }
  return interferenceGraph;
}

void CFG::computeRegisterAllocation() {
  LivenessInfo liveInfo = computeLiveInfo();
  std::map<std::shared_ptr<Symbol>, std::vector<std::shared_ptr<Symbol>>>
      interferenceGraph = buildInterferenceGraph(liveInfo);
  spillInformation spillInfo = findColorOrder(interferenceGraph, 7);

  registerAssignment = assignRegisters(spillInfo, interferenceGraph, 7);
}
