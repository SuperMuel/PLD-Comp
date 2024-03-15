#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
#include "ir.h"
#include "support/Any.h"

#include <string>

using namespace std;

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

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {

  std::string val = visit(ctx->expr()).as<std::string>();

  cfg.current_bb->add_IRInstr(IRInstr::ret, Type::INT, {val}, &cfg);

  return 0;
}

antlrcpp::Any Visitor::visitFunction_call_stmt(ifccParser::Function_call_stmt *ctx)
{
    return visit(ctx->function_call());
}

antlrcpp::Any Visitor::visitFunc(ifccParser::FuncContext *ctx)
{
    // Check if return value is void
    /*VarData returnedVar = visit(ctx->Function_call()).as<VarData>();

    std::string functionName = ctx->ID()->getText();
    Symbol *functionSymbol = getSymbol(ctx, functionName);
    if (functionSymbol == nullptr)
    {
        return 1;
    }

    // check if returnedVar is void
    if (returnedVar.type == Type::VOID)
    {
        std::string error = "Function " + functionName + " returns void";
        errorListener.addError(ctx, error, ErrorType::Error);
    }*/ 

    return visit(ctx->function_call());
}

antlrcpp::Any Visitor::visitFunction_call(ifccParser::Function_call *ctx)
{
    std::string functionName = ctx->ID()->getText();
    std::vector<std::string> args;

    // For now we only have one argument for putchar
    /*for (auto expr : ctx->expr())
    {
        args.push_back(visit(expr).as<std::string>());
    }*/
    arg = visit(ctx->expr()).as<std::string>();

    if (functionName == "putchar")
    {
      // Add an IR instruction to move the argument to %rdi before the call
      cfg.current_bb->add_IRInstr(IRInstr::move, Type::INT, {"%edi", argument}, &cfg);
        // Add an IR instruction for the call itself
      cfg.current_bb->add_IRInstr(IRInstr::call, Type::INT, {functionName}, &cfg);
    }
    else if (functionName == "getchar")
    {
      cfg.current_bb->add_IRInstr(IRInstr::call, Type::INT, {functionName}, &cfg);
      // Add an IR instruction to move the return value from %rax to a temporary variable
      std::string tempName = cfg.create_new_tempvar(Type::INT);
      cfg.current_bb->add_IRInstr(IRInstr::move, Type::INT, {tempName, "%eax"}, &cfg);
    }
    else
    {
      std::string error = "Function " + functionName + " not found";
      errorListener.addError(ctx, error, ErrorType::Error);
    }
    

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
  return visit(ctx->expr());
}

antlrcpp::Any CodeGenVisitor::visitMultdiv(ifccParser::MultdivContext *ctx) {
  IRInstr::Operation instr =
      (ctx->op->getText() == "*" ? IRInstr::mul : IRInstr::div);

  std::string leftVal = visit(ctx->expr(0)).as<std::string>();
  std::string rightVal = visit(ctx->expr(1)).as<std::string>();

  std::string tempName =
      cfg.current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal}, &cfg);

  return tempName;
}

antlrcpp::Any CodeGenVisitor::visitAddsub(ifccParser::AddsubContext *ctx) {
  IRInstr::Operation instr =
      (ctx->op->getText() == "+" ? IRInstr::add : IRInstr::sub);

  std::string leftVal = visit(ctx->expr(0)).as<std::string>();
  std::string rightVal = visit(ctx->expr(1)).as<std::string>();

  std::string tempName =
      cfg.current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal}, &cfg);

  return tempName;
}

antlrcpp::Any CodeGenVisitor::visitVal(ifccParser::ValContext *ctx) {
  std::string source;
  if (ctx->ID() != nullptr) {
    Symbol *symbol = getSymbol(ctx, ctx->ID()->toString());
    if (symbol != nullptr) {
      source = cfg.current_bb->add_IRInstr(IRInstr::ldvar, Type::INT,
                                           {ctx->ID()->toString()}, &cfg);
    }
  } else {
    source =
        cfg.current_bb->add_IRInstr(IRInstr::ldconst, Type::INT,
                                    {ctx->INTEGER_LITERAL()->toString()}, &cfg);
  }

  return source;
}

antlrcpp::Any CodeGenVisitor::visit

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
