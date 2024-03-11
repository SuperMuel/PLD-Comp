#pragma once

#include "ParserRuleContext.h"
#include "VisitorErrorListener.h"
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"

struct Symbol {
  // Memory offset (in bytes)
  int offset;
  // Wheter or not this variable has been used
  bool used;
  // In which line the symbol was declared
  int line;

  Symbol() : used(false), offset(0), line(1) {}
  Symbol(int line) : used(false), offset(0), line(line) {}
};

const std::string registers[] = {"r8d",  "r9d",  "r10d", "r11d",
                                 "r12d", "r13d", "r14d", "r15d"};

class CodeGenVisitor : public ifccBaseVisitor {
public:
  CodeGenVisitor() : freeRegister(0){};
  virtual ~CodeGenVisitor();

  virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;

  virtual antlrcpp::Any
  visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;

  virtual antlrcpp::Any
  visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) override;

  virtual antlrcpp::Any
  visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx) override;

  virtual antlrcpp::Any visitAdd(ifccParser::AddContext *ctx) override;

  virtual antlrcpp::Any visitSub(ifccParser::SubContext *ctx) override;

  virtual antlrcpp::Any visitDiv(ifccParser::DivContext *ctx) override;

  virtual antlrcpp::Any visitMult(ifccParser::MultContext *ctx) override;

  virtual antlrcpp::Any visitLiteral(ifccParser::LiteralContext *ctx) override;

  virtual antlrcpp::Any visitId(ifccParser::IdContext *ctx) override;

private:
  std::map<std::string, Symbol *> symbolTable;

  VisitorErrorListener errorListener;

  std::stringstream assembly;

  // Index of the free register with smallest index
  int freeRegister;

  bool addSymbol(antlr4::ParserRuleContext *ctx, const std::string &id);

  Symbol *getSymbol(antlr4::ParserRuleContext *ctx, const std::string &id);
};
