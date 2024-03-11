#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
#include "ir.h"
#include "support/Any.h"

using namespace std;

#include <string>

CodeGenVisitor::~CodeGenVisitor() {
  for (auto it = symbolTable.begin(); it != symbolTable.end(); it++) {
    delete it->second;
  }
}

CodeGenVisitor::CodeGenVisitor() : freeRegister(0) {
  BasicBlock *basicBlock = new BasicBlock(&cfg, "main");
  cfg.add_bb(basicBlock);
}

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
  /*assembly << ".globl main\n";
  assembly << " main: \n";

  assembly << "pushq %rbp\n";
  assembly << "movq %rsp, %rbp\n";*/

  for (ifccParser::StmtContext *stmt : ctx->stmt()) {
    this->visit(stmt);
  }

  this->visit(ctx->return_stmt());

  for (auto it = cfg.symbolTable.begin(); it != cfg.symbolTable.end(); it++) {
    if (!it->second.used) {
      errorListener.addError("Variable " + it->first +
                                 " not used (declared in line " +
                                 to_string(it->second->line) + ")",
                             ErrorType::Warning);
    }
  }

  if (errorListener.hasError()) {
    exit(1);
  }

  cout << assembly.str();

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) {
  addSymbol(ctx, ctx->ID()->toString());
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx) {
  Symbol *symbol = getSymbol(ctx, ctx->ID()->toString());

  if (symbol == nullptr) {
    return 1;
  }

  visitChildren(ctx);
  freeRegister--;

  assembly << "movl %" << registers[0] << ", -" << symbol->offset << "(%rbp)"
           << std::endl;

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitAdd(ifccParser::AddContext *ctx) {
  visit(ctx->expr());
  visit(ctx->term());

  freeRegister -= 2;

  assembly << "addl %" << registers[freeRegister + 1] << ", %"
           << registers[freeRegister] << std::endl;

  freeRegister++;

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitSub(ifccParser::SubContext *ctx) {
  visit(ctx->expr());
  visit(ctx->term());

  freeRegister -= 2;

  assembly << "subl %" << registers[freeRegister + 1] << ", %"
           << registers[freeRegister] << std::endl;

  freeRegister++;

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitMult(ifccParser::MultContext *ctx) {
  visit(ctx->term());
  visit(ctx->factor());

  freeRegister -= 2;

  assembly << "imull %" << registers[freeRegister + 1] << ", %"
           << registers[freeRegister] << std::endl;

  freeRegister++;

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitDiv(ifccParser::DivContext *ctx) {
  visit(ctx->term());
  visit(ctx->factor());

  // Division behaves a little bit differently, it divides the contents of
  // edx:eax (where ':' means concatenation) with the content of the given
  // register The quotient is stored in eax and the remainder in edx
  assembly << "movl %" << registers[freeRegister - 2] << ", %eax" << std::endl;
  assembly << "movl $0, %edx" << std::endl;
  assembly << "idivl %" << registers[freeRegister - 1] << std::endl;

  freeRegister -= 2;

  assembly << "movl %eax, %" << registers[freeRegister] << std::endl;

  freeRegister++;

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitLiteral(ifccParser::LiteralContext *ctx) {
  assembly << "movl $" << ctx->INTEGER_LITERAL()->toString() << ", %"
           << registers[freeRegister] << std::endl;

  freeRegister++;

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitId(ifccParser::IdContext *ctx) {
  Symbol *symbol = getSymbol(ctx, ctx->ID()->toString());
  if (symbol != nullptr) {
    assembly << "movl -" << symbol->offset << "(%rbp), %"
             << registers[freeRegister] << std::endl;
  }

  freeRegister++;
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
<<<<<<< HEAD
  visitChildren(ctx);

  assembly << "movl %" << registers[0] << ", %eax" << std::endl;
  assembly << "popq %rbp\n";
  assembly << "ret\n";

  return 0;
}

bool CodeGenVisitor::addSymbol(antlr4::ParserRuleContext *ctx,
                               const std::string &id) {
  if (symbolTable.count(id)) {
    string error = "The variable " + id + " has already been declared";
    errorListener.addError(ctx, error, ErrorType::Error);

    return false;
  }

  Symbol *newSymbol = new Symbol(ctx->getStart()->getLine());
  newSymbol->offset = 4 * (1 + symbolTable.size());
  symbolTable[id] = newSymbol;

  return true;
}

Symbol *CodeGenVisitor::getSymbol(antlr4::ParserRuleContext *ctx,
                                  const std::string &id) {
  auto it = symbolTable.find(id);
  if (it == symbolTable.end()) {
    const std::string error = "Symbol not found: " + id;
    errorListener.addError(ctx, error, ErrorType::Error);
    return nullptr;
  }

  it->second->used = true;
  return it->second;
}

CFG *CodeGenVisitor::getCfg() { return &cfg; }
