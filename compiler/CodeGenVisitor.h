#pragma once

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

class CodeGenVisitor : public ifccBaseVisitor {
public:
  virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;

  virtual antlrcpp::Any
  visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;

  virtual antlrcpp::Any
  visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) override;

  virtual antlrcpp::Any
  visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx) override;

private:
  std::map<std::string, Symbol> symbolTable;
  VisitorErrorListener errorListener;
  std::stringstream assembly;
};
