#include "CodeGenVisitor.h"

#include <any>
#include <string>

#include "Type.h"
#include "VisitorErrorListener.h"
#include "ir.h"
#include "support/Any.h"

using namespace std;

CodeGenVisitor::~CodeGenVisitor() {
  for (auto it = cfg.symbolTable.begin(); it != cfg.symbolTable.end(); it++) {
    delete it->second;
  }
}

CodeGenVisitor::CodeGenVisitor() {
  BasicBlock *basicBlock = new BasicBlock(&cfg, "");
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

antlrcpp::Any CodeGenVisitor::visitVar_decl_stmt(
    ifccParser::Var_decl_stmtContext *ctx) {
  Type type = typeMap[ctx->TYPE()->getText()];

  for (auto id : ctx->ID()) {
    addSymbol(ctx, id->toString(), type);
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitVar_assign_stmt(
    ifccParser::Var_assign_stmtContext *ctx) {
  
  Symbol *symbol = getSymbol(ctx, ctx->ID()->toString());

  if (symbol == nullptr) {
    return 1;
  }

  std::string source = visit(ctx->expr()).as<std::string>();

  cfg.current_bb->add_IRInstr(IRInstr::var_assign, symbol->type,{ctx->ID()->toString(), source}, &cfg);

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitVar_decl_assign_stmt(
    ifccParser::Var_decl_assign_stmtContext *ctx) {
  Type type = typeMap[ctx->TYPE()->getText()];

  for(auto assignContext : ctx->assignment()){
    
    if(!addSymbol(ctx, assignContext->ID()->toString(), type)){
      return 1;
    }

    std::pair <Symbol *, std::string> result = visitAssignment(assignContext);
    
    Symbol *symbol = result.first;
    std::string source = result.second;


    cfg.current_bb->add_IRInstr(IRInstr::var_assign, type,{assignContext->ID()->toString(), source}, &cfg);
  }
  
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitIf_stmt(ifccParser::If_stmtContext *ctx) {
  std::string result = visit(ctx->expr()).as<std::string>();
  BasicBlock *baseBlock = cfg.current_bb;
  BasicBlock *trueBlock = new BasicBlock(&cfg, "");
  cfg.add_bb(trueBlock);
  std::string nextBBLabel = ".L" + std::to_string(nextLabel);
  nextLabel++;
  BasicBlock *falseBlock = new BasicBlock(&cfg, nextBBLabel);
  trueBlock->exit_true = falseBlock;
  falseBlock->exit_true = baseBlock->exit_true;
  for (auto *stmt : ctx->stmt()) {
    visit(stmt);
  }
  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result, falseBlock->label},
                         &cfg);

  cfg.add_bb(falseBlock);
  baseBlock->exit_true = trueBlock;
  baseBlock->exit_false = falseBlock;
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(
    ifccParser::Return_stmtContext *ctx) {
  std::string val = visit(ctx->expr()).as<std::string>();
  cfg.current_bb->add_IRInstr(IRInstr::ret, Type::INT, {val}, &cfg);

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

antlrcpp::Any CodeGenVisitor::visitCmp(ifccParser::CmpContext *ctx) {
  IRInstr::Operation instr;
  if (ctx->op->getText() == ">") {
    instr = IRInstr::Operation::gt;
  } else if (ctx->op->getText() == ">=") {
    instr = IRInstr::Operation::geq;
  } else if (ctx->op->getText() == "<") {
    instr = IRInstr::Operation::lt;
  } else if (ctx->op->getText() == "<=") {
    instr = IRInstr::Operation::leq;
  }

  std::string leftVal = visit(ctx->expr(0)).as<std::string>();
  std::string rightVal = visit(ctx->expr(1)).as<std::string>();

  std::string tempName =
      cfg.current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal}, &cfg);

  return tempName;
}

antlrcpp::Any CodeGenVisitor::visitEq(ifccParser::EqContext *ctx) {
  IRInstr::Operation instr =
      (ctx->op->getText() == "==" ? IRInstr::eq : IRInstr::neq);

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

antlrcpp::Any CodeGenVisitor::visitAssignment(
    ifccParser::AssignmentContext *ctx) {

  Symbol *symbol = getSymbol(ctx, ctx->ID()->toString());
  
  std::string source = visit(ctx->expr()).as<std::string>();

  return std::pair<Symbol *, std::string>(symbol, source);
}

bool CodeGenVisitor::addSymbol(antlr4::ParserRuleContext *ctx,
                               const std::string &id, const Type &type) {
  // if the variable has already been declared
  if (cfg.symbolTable.count(id)) {
    string error = "The variable " + id + " has already been declared";
    errorListener.addError(ctx, error, ErrorType::Error);

    return false;
  }

  // add the variable to the symbol table
  Symbol *newSymbol = new Symbol(ctx->getStart()->getLine());
  newSymbol->type = type;
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
