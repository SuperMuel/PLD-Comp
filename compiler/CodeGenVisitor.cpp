#include "CodeGenVisitor.h"
#include "Type.h"
#include "VisitorErrorListener.h"
#include "ir.h"
#include "support/Any.h"

#include <string>

using namespace std;

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

  cfg.pop_table();

  if (VisitorErrorListener::hasError()) {
    exit(1);
  }

  cout << assembly.str();

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitVar_decl_stmt(ifccParser::Var_decl_stmtContext* ctx) {
    Type type;
    if (ctx->TYPE()->toString() == "int") {
        type = Type::INT;
    } else if (ctx->TYPE()->toString() == "char") {
        type = Type::CHAR;
    }
    // Iterate over each var_decl_member
    for (auto& memberCtx : ctx->var_decl_member()) {
        std::string varName = memberCtx->ID()->toString();
        addSymbol(memberCtx, varName, type); // Declare the variable

        if (memberCtx->expr()) { // Check for initialization
            std::shared_ptr<Symbol> symbol = getSymbol(memberCtx, varName);
            std::shared_ptr<Symbol> source = visit(memberCtx->expr()).as<std::shared_ptr<Symbol>>();
            cfg.current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, source});
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

  cfg.current_bb->add_IRInstr(IRInstr::var_assign, Type::INT, {symbol, source});
  return 0;
}

antlrcpp::Any CodeGenVisitor::visitIf(ifccParser::IfContext *ctx) {
  std::shared_ptr<Symbol> result =
      visit(ctx->expr()).as<std::shared_ptr<Symbol>>();
  std::string nextBBLabel = ".L" + std::to_string(nextLabel);
  nextLabel++;

  BasicBlock *baseBlock = cfg.current_bb;
  BasicBlock *trueBlock = new BasicBlock(&cfg, "");
  BasicBlock *falseBlock = new BasicBlock(&cfg, nextBBLabel);

  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  trueBlock->exit_true = falseBlock;
  falseBlock->exit_true = baseBlock->exit_true;

  cfg.add_bb(trueBlock);
  visit(ctx->block());

  cfg.add_bb(falseBlock);

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

  BasicBlock *baseBlock = cfg.current_bb;
  BasicBlock *trueBlock = new BasicBlock(&cfg, "");
  BasicBlock *elseBlock = new BasicBlock(&cfg, elseBBLabel);
  BasicBlock *endBlock = new BasicBlock(&cfg, endBBLabel);

  trueBlock->exit_true = endBlock;
  elseBlock->exit_true = endBlock;
  endBlock->exit_true = baseBlock->exit_true;

  baseBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  cfg.add_bb(trueBlock);
  visit(ctx->if_block);

  cfg.add_bb(elseBlock);
  visit(ctx->else_block);

  cfg.add_bb(endBlock);

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

  BasicBlock *baseBlock = cfg.current_bb;
  BasicBlock *conditionBlock = new BasicBlock(&cfg, conditionBBLabel);
  BasicBlock *stmtBlock = new BasicBlock(&cfg, "");
  BasicBlock *endBlock = new BasicBlock(&cfg, endBBLabel);

  conditionBlock->exit_true = stmtBlock;
  conditionBlock->exit_false = endBlock;
  endBlock->exit_true = baseBlock->exit_true;
  stmtBlock->exit_true = conditionBlock;
  baseBlock->exit_true = conditionBlock;

  cfg.add_bb(conditionBlock);
  std::shared_ptr<Symbol> result =
      visit(ctx->expr()).as<std::shared_ptr<Symbol>>();
  conditionBlock->add_IRInstr(IRInstr::cmpNZ, Type::INT, {result});

  cfg.add_bb(stmtBlock);
  visit(ctx->block());

  cfg.add_bb(endBlock);

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx) {
  cfg.push_table();
  for (ifccParser::StmtContext *stmt : ctx->stmt()) {
    visit(stmt);
  }

  cfg.pop_table();
  return 0;
}

antlrcpp::Any
CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
  std::shared_ptr<Symbol> val =
      visit(ctx->expr()).as<std::shared_ptr<Symbol>>();
  cfg.current_bb->add_IRInstr(IRInstr::ret, Type::INT, {val});

  return 0;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
  return visit(ctx->expr());
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

  return cfg.current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitAddsub(ifccParser::AddsubContext *ctx) {
  IRInstr::Operation instr =
      (ctx->op->getText() == "+" ? IRInstr::add : IRInstr::sub);

  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  return cfg.current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
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

  return cfg.current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitEq(ifccParser::EqContext *ctx) {
  IRInstr::Operation instr =
      (ctx->op->getText() == "==" ? IRInstr::eq : IRInstr::neq);

  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  return cfg.current_bb->add_IRInstr(instr, Type::INT, {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitVal(ifccParser::ValContext *ctx) {
  std::shared_ptr<Symbol> source;
  if (ctx->ID() != nullptr) {
    std::shared_ptr<Symbol> symbol = getSymbol(ctx, ctx->ID()->toString());
    if (symbol != nullptr) {
      source = cfg.current_bb->add_IRInstr(IRInstr::ldvar, Type::INT, {symbol});
    }
  } else if (ctx->INTEGER_LITERAL() != nullptr) {
    source = cfg.current_bb->add_IRInstr(IRInstr::ldconst, Type::INT,
                                         {ctx->INTEGER_LITERAL()->toString()});
  } else if (ctx->CHAR_LITERAL() != nullptr) {
    std::string val =
        std::to_string(static_cast<int>(ctx->CHAR_LITERAL()->toString()[1]));
    source = cfg.current_bb->add_IRInstr(IRInstr::ldconst, Type::CHAR, {val});
  }

  return source;
}

bool CodeGenVisitor::addSymbol(antlr4::ParserRuleContext *ctx,
                               const std::string &id, Type type) {
  bool result = cfg.add_symbol(id, type, ctx->getStart()->getLine());
  if (!result) {
    std::string error = "The variable " + id + " has already been declared";
    VisitorErrorListener::addError(ctx, error, ErrorType::Error);
  }
  return result;
}

std::shared_ptr<Symbol>
CodeGenVisitor::getSymbol(antlr4::ParserRuleContext *ctx,
                          const std::string &id) {
  std::shared_ptr<Symbol> symbol = cfg.get_symbol(id);
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

  return cfg.current_bb->add_IRInstr(IRInstr::b_and, Type::INT,
                                     {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitB_or(ifccParser::B_orContext *ctx) {
  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  return cfg.current_bb->add_IRInstr(IRInstr::b_or, Type::INT,
                                     {leftVal, rightVal});
}

antlrcpp::Any CodeGenVisitor::visitB_xor(ifccParser::B_xorContext *ctx) {
  std::shared_ptr<Symbol> leftVal =
      visit(ctx->expr(0)).as<std::shared_ptr<Symbol>>();
  std::shared_ptr<Symbol> rightVal =
      visit(ctx->expr(1)).as<std::shared_ptr<Symbol>>();

  return cfg.current_bb->add_IRInstr(IRInstr::b_xor, Type::INT,
                                     {leftVal, rightVal});
}
