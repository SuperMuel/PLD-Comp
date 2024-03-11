#pragma once

#include "ParserRuleContext.h"
#include "Symbol.h"
#include "VisitorErrorListener.h"
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "ir.h"

const std::string registers[] = {"r8d",  "r9d",  "r10d", "r11d",
                                 "r12d", "r13d", "r14d", "r15d"};

class CodeGenVisitor : public ifccBaseVisitor {
public:
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

  CFG *getCfg();

private:
  std::map<std::string, Symbol *> symbolTable;

  VisitorErrorListener errorListener;

  VisitorErrorListener errorListener;
  CFG cfg;
  std::stringstream assembly;

  // Index of the free register with smallest index
  int freeRegister;

  bool addSymbol(antlr4::ParserRuleContext *ctx, const std::string &id);

  Symbol *getSymbol(antlr4::ParserRuleContext *ctx, const std::string &id);
};
