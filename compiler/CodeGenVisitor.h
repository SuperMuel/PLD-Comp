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
  virtual antlrcpp::Any visitFunc(ifccParser::FuncContext *ctx) override;

  virtual antlrcpp::Any
  visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;

  virtual antlrcpp::Any
  visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) override;

  virtual antlrcpp::Any
  visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx) override;

  virtual antlrcpp::Any visitFunction_call(ifccParser::Function_callContext *ctx) override;

  virtual antlrcpp::Any visitFunction_call_expr(ifccParser::Function_call_exprContext *ctx) override;

  virtual antlrcpp::Any visitFunction_call_stmt(ifccParser::Function_call_stmtContext *ctx) override;

  virtual antlrcpp::Any visitPar(ifccParser::ParContext *ctx) override;

  virtual antlrcpp::Any visitIf(ifccParser::IfContext *ctx) override;

  virtual antlrcpp::Any visitIf_else(ifccParser::If_elseContext *ctx) override;

  virtual antlrcpp::Any
  visitWhile_stmt(ifccParser::While_stmtContext *ctx) override;

  virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;

  virtual antlrcpp::Any visitMultdiv(ifccParser::MultdivContext *ctx) override;

  virtual antlrcpp::Any visitAddsub(ifccParser::AddsubContext *ctx) override;

  virtual antlrcpp::Any visitCmp(ifccParser::CmpContext *ctx) override;

  virtual antlrcpp::Any visitEq(ifccParser::EqContext *ctx) override;

  virtual antlrcpp::Any visitVal(ifccParser::ValContext *ctx) override;

  inline CFG *const getCfg() { return &cfg; };

  virtual antlrcpp::Any visitB_and(ifccParser::B_andContext *ctx) override;

  virtual antlrcpp::Any visitB_or(ifccParser::B_orContext *ctx) override;

  virtual antlrcpp::Any visitB_xor(ifccParser::B_xorContext *ctx) override;

private:
  VisitorErrorListener errorListener;
  // Keeps track of the label for the next jump
  int nextLabel = 1;
  CFG cfg;
  std::stringstream assembly;

  bool addSymbol(antlr4::ParserRuleContext *ctx, const std::string &id);

  Symbol *getSymbol(antlr4::ParserRuleContext *ctx, const std::string &id);
};
