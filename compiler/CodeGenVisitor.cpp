#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
#include "ir.h"
#include "support/Any.h"
#include <any>

using namespace std;

#include <string>

CodeGenVisitor::~CodeGenVisitor() {
  for (auto it = cfg.symbolTable.begin(); it != cfg.symbolTable.end(); it++) {
    delete it->second;
  }
}

CodeGenVisitor::CodeGenVisitor() {
  BasicBlock *basicBlock = new BasicBlock(&cfg, "main");
  cfg.add_bb(basicBlock);
}

antlrcpp::Any CodeGenVisitor::visitAxiom(ifccParser::AxiomContext *ctx) {
  return this->visit(ctx->prog());
}

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
  for (ifccParser::StmtContext *stmt : ctx->stmt()) {
    this->visit(stmt);
  }

  this->visit(ctx->return_stmt());

  for (auto it = cfg.symbolTable.begin(); it != cfg.symbolTable.end(); it++) {
    if (!it->second->used) {
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

  std::string source = visit(ctx->expr()).as<std::string>();

  cfg.current_bb->add_IRInstr(IRInstr::var_assign, Type::INT,
                              {ctx->ID()->toString(), source}, &cfg);
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddsub(ifccParser::AddsubContext *ctx) {
  std::string leftVal = visit(ctx->expr()).as<std::string>();
  std::string rightVal = visit(ctx->term()).as<std::string>();

  std::string tempName = cfg.current_bb->add_IRInstr(IRInstr::add, Type::INT,
                                                     {leftVal, rightVal}, &cfg);

  return antlrcpp::Any(tempName);
}

antlrcpp::Any CodeGenVisitor::visitSub(ifccParser::SubContext *ctx) {
  antlrcpp::Any temp = visit(ctx->expr());
  std::string leftVal = temp.as<std::string>();
  std::string rightVal = visit(ctx->term()).as<std::string>();

  std::string tempName = cfg.current_bb->add_IRInstr(IRInstr::sub, Type::INT,
                                                     {leftVal, rightVal}, &cfg);

  return antlrcpp::Any(tempName);
}

antlrcpp::Any CodeGenVisitor::visitMultdiv(ifccParser::MultdivContext *ctx) {
  visit(ctx->expr(0));
  visit(ctx->expr(1));

  if (ctx->op->getText() == "*") {
    freeRegister -= 2;
    assembly << "imull %" << registers[freeRegister + 1] << ", %"
             << registers[freeRegister] << std::endl;
    freeRegister++;
  } else {
    // Division behaves a little bit differently, it divides the contents of
    // edx:eax (where ':' means concatenation) with the content of the given
    // register The quotient is stored in eax and the remainder in edx
    assembly << "movl %" << registers[freeRegister - 2] << ", %eax"
             << std::endl;
    assembly << "movl $0, %edx" << std::endl;
    assembly << "idivl %" << registers[freeRegister - 1] << std::endl;

    freeRegister -= 2;

    assembly << "movl %eax, %" << registers[freeRegister] << std::endl;

    freeRegister++;
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitVal(ifccParser::ValContext *ctx) {
  if (ctx->ID() != nullptr) {
    Symbol *symbol = getSymbol(ctx, ctx->ID()->toString());
    if (symbol != nullptr) {
      assembly << "movl -" << symbol->offset << "(%rbp), %"
               << registers[freeRegister] << std::endl;
    }
  } else {
    assembly << "movl $" << ctx->INTEGER_LITERAL()->toString() << ", %"
             << registers[freeRegister] << std::endl;
  }

  freeRegister++;

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {

  std::string val = visit(ctx->expr()).as<std::string>();

  cfg.current_bb->add_IRInstr(IRInstr::ret, Type::INT, {val}, &cfg);

  return 0;
}

bool CodeGenVisitor::addSymbol(antlr4::ParserRuleContext *ctx,
                               const std::string &id) {
  if (cfg.symbolTable.count(id)) {
    string error = "The variable " + id + " has already been declared";
    errorListener.addError(ctx, error, ErrorType::Error);

    return false;
  }

  Symbol *newSymbol = new Symbol(ctx->getStart()->getLine());
  newSymbol->offset = 4 * (1 + cfg.symbolTable.size());
  cfg.symbolTable[id] = newSymbol;

  return true;
}

Symbol *CodeGenVisitor::getSymbol(antlr4::ParserRuleContext *ctx,
                                  const std::string &id) {
  auto it = cfg.symbolTable.find(id);
  if (it == cfg.symbolTable.end()) {
    const std::string error = "Symbol not found: " + id;
    errorListener.addError(ctx, error, ErrorType::Error);
    return nullptr;
  }

  it->second->used = true;
  return it->second;
}

CFG *CodeGenVisitor::getCfg() { return &cfg; }
