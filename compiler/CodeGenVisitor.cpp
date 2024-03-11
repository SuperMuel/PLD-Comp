#include "CodeGenVisitor.h"
#include "VisitorErrorListener.h"

using namespace std;

#include <string>

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
  assembly << ".globl main\n";
  assembly << " main: \n";

  assembly << "pushq %rbp\n";
  assembly << "movq %rsp, %rbp\n";

  for (ifccParser::StmtContext *stmt : ctx->stmt()) {
    this->visit(stmt);
  }

  this->visit(ctx->return_stmt());

  for (auto it = symbolTable.begin(); it != symbolTable.end(); it++) {
    if (!it->second.used) {
      errorListener.addError("Variable " + it->first +
                                 " not used (declared in line " +
                                 to_string(it->second.line) + ")",
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
  if (symbolTable.count(ctx->ID()->toString())) {
    string error =
        "The variable " + ctx->ID()->toString() + " has already been declared";
    errorListener.addError(ctx, error, ErrorType::Error);

    return 1;
  }

  Symbol newSymbol(ctx->getStart()->getLine());
  newSymbol.offset = 4 * (1 + symbolTable.size());
  symbolTable[ctx->ID()->toString()] = newSymbol;

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx) {
  if (!symbolTable.count(ctx->ID(0)->toString())) {
    string error = "Symbol not found: " + ctx->ID(0)->toString();
    errorListener.addError(ctx, error, ErrorType::Error);
    return 1;
  }

  Symbol &symbol1 = symbolTable[ctx->ID(0)->toString()];
  symbol1.used = true;

  if (ctx->INTEGER_LITERAL() != nullptr) {
    assembly << "movl $" << ctx->INTEGER_LITERAL()->toString() << ", -" << symbol1.offset
             << "(%rbp)" << std::endl;
    return 0;
  }

  if (!symbolTable.count(ctx->ID(1)->toString())) {
    string error = "Symbol not found: " + ctx->ID(0)->toString();
    errorListener.addError(ctx, error, ErrorType::Error);
    return 1;
  }

  Symbol &symbol2 = symbolTable[ctx->ID(1)->toString()];
  symbol2.used = true;

  assembly << "movl -" << symbol2.offset << "(%rbp)"
           << ", %eax" << std::endl;
  assembly << "movl %eax"
           << ", -" << symbol1.offset << "(%rbp)" << std::endl;

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
  int retval = stoi(ctx->INTEGER_LITERAL()->getText());

  assembly << "    movl $" << retval << ", %eax\n";

  assembly << "popq %rbp\n";
  assembly << "ret\n";

  return 0;
}
