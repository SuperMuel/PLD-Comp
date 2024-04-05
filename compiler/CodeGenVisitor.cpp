#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
#include "ir.h"
#include "support/Any.h"

#include <string>

using namespace std;

CodeGenVisitor::CodeGenVisitor() {}

antlrcpp::Any CodeGenVisitor::visitAxiom(ifccParser::AxiomContext *ctx) {
  return visit(ctx->prog());
}

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
  for (ifccParser::FuncContext *func : ctx->func()) {
    Type type;
    if (func->TYPE(0)->toString() == "int") {
      type = Type::INT;
    } else if (func->TYPE(0)->toString() == "char") {
      type = Type::CHAR;
    } else if (func->TYPE(0)->toString() == "void") {
      type = Type::VOID;
    }
    curCfg = std::make_shared<CFG>(type, func->ID(0)->toString(), this);
    cfgList.push_back(curCfg);
    functions[func->ID(0)->toString()] = curCfg;
    visit(func);
    curCfg->pop_table();
  }

  if (VisitorErrorListener::hasError()) {
    exit(1);
  }

  cout << assembly.str();

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunc(ifccParser::FuncContext *ctx) {
  for (int i = 1; i < ctx->ID().size(); i++) {
    Type type = (ctx->TYPE(i)->toString() == "int" ? Type::INT : Type::CHAR);
    auto symbol = curCfg->add_parameter(ctx->ID(i)->toString(), type,
                                        ctx->getStart()->getLine());
    curCfg->current_bb->add_IRInstr(IRInstr::param_decl, type, {symbol});
  }

  for (ifccParser::StmtContext *stmt : ctx->block()->stmt()) {
    visit(stmt);
  }

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitVar_decl_stmt(ifccParser::Var_decl_stmtContext *ctx) {
  Type type;
  if (ctx->TYPE()->toString() == "int") {
    type = Type::INT;
  } else if (ctx->TYPE()->toString() == "char") {
    type = Type::CHAR;
  } else if (ctx->TYPE()->toString() == "void") {
    VisitorErrorListener::addError(ctx, "Can't create a variable of type void");
  }
  // Iterate over each var_decl_member
  for (auto &memberCtx : ctx->var_decl_member()) {
    std::string varName = memberCtx->ID()->toString();
    addSymbol(memberCtx, varName, type); // Declare the variable

    if (memberCtx->expr()) { // Check for initialization
      std::shared_ptr<Symbol> symbol = getSymbol(memberCtx, varName);
      std::shared_ptr<Symbol> source =
          visit(memberCtx->expr()).as<std::shared_ptr<Symbol>>();
      curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT,
                                      {symbol, source});
    }
  }

  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitVar_assign_stmt(ifccParser::Var_assign_stmtContext *ctx) {
  std::shared_ptr<Symbol> symbol = getSymbol(ctx, ctx->ID()->toString());

  if (symbol == nullptr) {
    return 1;
  }

  std::shared_ptr<Symbol> source =
      visit(ctx->expr()).as<std::shared_ptr<Symbol>>();

  curCfg->current_bb->add_IRInstr(IRInstr::var_assign, Type::INT,
                                  {symbol, source});
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitIf(ifccParser::IfContext *ctx) {
  std::shared_ptr<Symbol> result =
      visit(ctx->expr()).as<std::shared_ptr<Symbol>>();
  std::string nextBBLabel = ".L" + std::to_string(nextLabel);
  nextLabel++;

  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *trueBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *falseBlock = new BasicBlock(curCfg.get(), nextBBLabel);

  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  trueBlock->exit_true = falseBlock;
  falseBlock->exit_true = baseBlock->exit_true;

  curCfg->add_bb(trueBlock);
  visit(ctx->block());

  curCfg->add_bb(falseBlock);

  baseBlock->exit_true = trueBlock;
  baseBlock->exit_false = falseBlock;
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitIf_else(ifccParser::If_elseContext *ctx) {
  std::shared_ptr<Symbol> result =
      visit(ctx->expr()).as<std::shared_ptr<Symbol>>();
  std::string elseBBLabel = ".L" + std::to_string(nextLabel);
  nextLabel++;
  std::string endBBLabel = ".L" + std::to_string(nextLabel);
  nextLabel++;

  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *trueBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *elseBlock = new BasicBlock(curCfg.get(), elseBBLabel);
  BasicBlock *endBlock = new BasicBlock(curCfg.get(), endBBLabel);

  trueBlock->exit_true = endBlock;
  elseBlock->exit_true = endBlock;
  endBlock->exit_true = baseBlock->exit_true;

  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  curCfg->add_bb(trueBlock);
  visit(ctx->if_block);

  curCfg->add_bb(elseBlock);
  visit(ctx->else_block);

  curCfg->add_bb(endBlock);

  baseBlock->exit_true = trueBlock;
  baseBlock->exit_false = elseBlock;
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitWhile_stmt(ifccParser::While_stmtContext *ctx) {
  std::string conditionBBLabel = ".L" + std::to_string(nextLabel);
  nextLabel++;
  std::string endBBLabel = ".L" + std::to_string(nextLabel);
  nextLabel++;

  BasicBlock *baseBlock = curCfg->current_bb;
  BasicBlock *conditionBlock = new BasicBlock(curCfg.get(), conditionBBLabel);
  BasicBlock *stmtBlock = new BasicBlock(curCfg.get(), "");
  BasicBlock *endBlock = new BasicBlock(curCfg.get(), endBBLabel);

  conditionBlock->exit_true = stmtBlock;
  conditionBlock->exit_false = endBlock;
  endBlock->exit_true = baseBlock->exit_true;
  stmtBlock->exit_true = conditionBlock;
  baseBlock->exit_true = conditionBlock;

  curCfg->add_bb(conditionBlock);
  std::shared_ptr<Symbol> result =
      visit(ctx->expr()).as<std::shared_ptr<Symbol>>();
  conditionBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  curCfg->add_bb(stmtBlock);
  visit(ctx->block());

  curCfg->add_bb(endBlock);

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx) {
  curCfg->push_table();
  for (ifccParser::StmtContext *stmt : ctx->stmt()) {
    visit(stmt);
  }

  curCfg->pop_table();
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
  if (curCfg->get_return_type() != Type::VOID) {
    if (ctx->expr() == nullptr) {
      std::string message =
          "Non void function " + curCfg->get_name() + " should return a value";
      VisitorErrorListener::addError(ctx, message);
      return 1;
    }
    std::shared_ptr<Symbol> val =
        visit(ctx->expr()).as<std::shared_ptr<Symbol>>();
    curCfg->current_bb->add_IRInstr(IRInstr::ret, curCfg->get_return_type(),
                                    {val});
  } else {
    if (ctx->expr() != nullptr) {
      std::string message =
          "Void function " + curCfg->get_name() + " should not return a value";
      VisitorErrorListener::addError(ctx, message);
      return 1;
    }
    curCfg->current_bb->add_IRInstr(IRInstr::ret, curCfg->get_return_type(),
                                    {});
  }

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
  return visit(ctx->expr());
}

antlrcpp::Any
CodeGenVisitor::visitFunc_call(ifccParser::Func_callContext *ctx) {
  auto it = functions.find(ctx->ID()->toString());
  if (it == functions.end()) {
    std::string message =
        "Function " + ctx->ID()->toString() + " has not been declared";
    VisitorErrorListener::addError(ctx, message);
  }

  auto funcCfg = it->second;

  if (ctx->expr().size() != funcCfg->get_parameters_type().size()) {
    std::string message = "Wrong number of parameters in function call to " +
                          funcCfg->get_name() + ": expected " +
                          to_string(funcCfg->get_parameters_type().size()) +
                          " but found " + to_string(ctx->expr().size()) +
                          " instead";
    VisitorErrorListener::addError(ctx, message);
  }

  std::vector<Parameter> params = {ctx->ID()->toString()};
  for (int i = 0; i < funcCfg->get_parameters_type().size(); i++) {
    std::shared_ptr<Symbol> symbol =
        visit(ctx->expr(i)).as<std::shared_ptr<Symbol>>();
    params.push_back(symbol);
    curCfg->current_bb->add_IRInstr(
        IRInstr::param, funcCfg->get_parameters_type()[i].type, {symbol});
  }

  return curCfg->current_bb->add_IRInstr(IRInstr::call,
                                         funcCfg->get_return_type(), params);
}

antlrcpp::Any CodeGenVisitor::visitMultdiv(ifccParser::MultdivContext *ctx) {
  IRInstr::Operation instr;
  if (ctx->op->getText() == "*") {
    instr = IRInstr::mul;
  } else if (ctx->op->getText() == "/") {
    instr = IRInstr::div;
  } else {
    instr = IRInstr::mod;
  }

  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitAddsub(ifccParser::AddsubContext *ctx) {
  IRInstr::Operation instr =
      (ctx->op->getText() == "+" ? IRInstr::add : IRInstr::sub);

  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr) {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
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

  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr) {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitEq(ifccParser::EqContext *ctx) {
  IRInstr::Operation instr =
      (ctx->op->getText() == "==" ? IRInstr::eq : IRInstr::neq);

  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr) {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitVal(ifccParser::ValContext *ctx) {
  std::shared_ptr<Symbol> source;
  if (ctx->ID() != nullptr) {
    std::shared_ptr<Symbol> symbol = getSymbol(ctx, ctx->ID()->toString());
    if (symbol != nullptr) {
      source =
          curCfg->current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbol});
    }
  } else if (ctx->INTEGER_LITERAL() != nullptr) {
    source = curCfg->current_bb->add_IRInstr(
        IRInstr::ldconst, Type::INT, {ctx->INTEGER_LITERAL()->toString()});
  } else if (ctx->CHAR_LITERAL() != nullptr) {
    std::string val =
        std::to_string(static_cast<int>(ctx->CHAR_LITERAL()->toString()[1]));
    source =
        curCfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::CHAR, {val});
  }

  return source;
}

bool CodeGenVisitor::addSymbol(antlr4::ParserRuleContext *ctx,
                               const std::string &id, Type type) {
  bool result = curCfg->add_symbol(id, type, ctx->getStart()->getLine());
  if (!result) {
    std::string error = "The variable " + id + " has already been declared";
    VisitorErrorListener::addError(ctx, error, ErrorType::Error);
  }
  return result;
}

std::shared_ptr<Symbol>
CodeGenVisitor::getSymbol(antlr4::ParserRuleContext *ctx,
                          const std::string &id) {
  std::shared_ptr<Symbol> symbol = curCfg->get_symbol(id);
  if (symbol == nullptr) {
    const std::string error = "Symbol not found: " + id;
    VisitorErrorListener::addError(ctx, error, ErrorType::Error);
    return nullptr;
  }

  symbol->used = true;
  return symbol;
}

antlrcpp::Any CodeGenVisitor::visitB_and(ifccParser::B_andContext *ctx) {
  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  return curCfg->current_bb->add_IRInstr(IRInstr::b_and, Type::INT,
                                         {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitB_or(ifccParser::B_orContext *ctx) {
  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr) {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(IRInstr::b_or, Type::INT,
                                         {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitB_xor(ifccParser::B_xorContext *ctx) {
  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  if (leftVal == nullptr || rightVal == nullptr) {
    VisitorErrorListener::addError(
        ctx, "Invalid operation with function returning void");
  }

  return curCfg->current_bb->add_IRInstr(IRInstr::b_xor, Type::INT,
                                         {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitUnaryOp(ifccParser::UnaryOpContext *ctx) {
  std::shared_ptr<Symbol> val =
      visit(ctx->expr()).as<std::shared_ptr<Symbol>>();
  IRInstr::Operation instr;
  if (ctx->op->getText() == "-") {
    instr = IRInstr::neg;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  } else if (ctx->op->getText() == "~") {
    instr = IRInstr::not_;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  } else if (ctx->op->getText() == "!") {
    instr = IRInstr::lnot;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  } else if (ctx->op->getText() == "++") {
    instr = IRInstr::inc;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  } else if (ctx->op->getText() == "--") {
    instr = IRInstr::dec;
    return curCfg->current_bb->add_IRInstr(instr, Type::INT, {val});
  } else if (ctx->op->getText() == "+") {
    return val;
  }
  return nullptr;
}
