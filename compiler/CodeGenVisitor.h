#pragma once

#include "ParserRuleContext.h"
#include "Symbol.h"
#include "VisitorErrorListener.h"
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "ir.h"

class CodeGenVisitor : public ifccBaseVisitor {
public:
  virtual ~CodeGenVisitor();
  CodeGenVisitor();

  virtual antlrcpp::Any visitAxiom(ifccParser::AxiomContext *ctx) override;
  virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;

  virtual antlrcpp::Any
  visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;

  virtual antlrcpp::Any
  visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) override;

  virtual antlrcpp::Any
  visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx) override;

  virtual antlrcpp::Any visitPar(ifccParser::ParContext *ctx) override;
  virtual antlrcpp::Any visitIf_stmt(ifccParser::If_stmtContext *ctx) override;

  virtual antlrcpp::Any visitMultdiv(ifccParser::MultdivContext *ctx) override;

  virtual antlrcpp::Any visitAddsub(ifccParser::AddsubContext *ctx) override;

  virtual antlrcpp::Any visitCmp(ifccParser::CmpContext *ctx) override;

  virtual antlrcpp::Any visitEq(ifccParser::EqContext *ctx) override;

  virtual antlrcpp::Any visitVal(ifccParser::ValContext *ctx) override;

  inline CFG *const getCfg() { return &cfg; };

private:
  VisitorErrorListener errorListener;
  // Keeps track of the label for the next jump
  int nextLabel = 1;
  CFG cfg;
  std::stringstream assembly;

  bool addSymbol(antlr4::ParserRuleContext *ctx, const std::string &id);

  Symbol *getSymbol(antlr4::ParserRuleContext *ctx, const std::string &id);
};
