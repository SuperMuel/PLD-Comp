#pragma once
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "ParserRuleContext.h"
#include "Symbol.h"
#include "Type.h"
#include "VisitorErrorListener.h"

class BasicBlock;
class CFG;

typedef std::map<std::string, std::shared_ptr<Symbol>> SymbolTable;
typedef std::variant<std::shared_ptr<Symbol>, std::string> Parameter;

const std::string registers8[] = {"r8b",  "r9b",  "r10b", "r11b",
                                  "r12b", "r13b", "r14b", "r15b"};
const std::string registers32[] = {"r8d",  "r9d",  "r10d", "r11d",
                                   "r12d", "r13d", "r14d", "r15d"};

std::ostream &operator<<(std::ostream &os, const Parameter &param);

class IRInstr {

public:
  /** The instructions themselves -- feel free to subclass instead */
  typedef enum {
    var_assign,
    ldconst,
    ldvar,
    add,
    sub,
    mul,
    div,
    mod,
    b_and,
    b_or,
    b_xor,
    cmpNZ,
    ret,
    leq,
    lt,
    geq,
    gt,
    eq,
    neq,
    neg,
    not_,
    lnot, 
    inc
  } Operation;

  /**  constructor */
  IRInstr(BasicBlock *bb_, Operation op, Type t,
          const std::vector<Parameter> &params);

  void genAsm(std::ostream &os, CFG *cfg);

  friend std::ostream &operator<<(std::ostream &os, IRInstr &instruction);

private:
  Type outType;
  std::vector<Parameter> params;
  Operation op;
  BasicBlock *block;

  // Functions to generate the assembly
  void handleCmpNZ(std::ostream &os, CFG *cfg);
  void handleDiv(std::ostream &os, CFG *cfg);
  void handleMod(std::ostream &os, CFG *cfg);
  void handleRet(std::ostream &os, CFG *cfg);
  void handleVar_assign(std::ostream &os, CFG *cfg);
  void handleLdconst(std::ostream &os, CFG *cfg);
  void handleLdvar(std::ostream &os, CFG *cfg);
  void handleUnaryOp(const std::string &op, std::ostream &os, CFG *cfg);
  void handleBinaryOp(const std::string &op, std::ostream &os, CFG *cfg);
  void handleCmpOp(const std::string &op, std::ostream &os, CFG *cfg);
};

class BasicBlock {
public:
  BasicBlock(CFG *cfg, std::string entry_label);
  void gen_asm(std::ostream &
                   o); /**< x86 assembly code generation for this
                                                    basic block (very simple) */
  std::shared_ptr<Symbol> add_IRInstr(IRInstr::Operation op, Type t,
                                      std::vector<Parameter> params);

  // No encapsulation whatsoever here. Feel free to do better.
  /**< pointer to the next basic block, true branch. If
  nullptr the basic block ends with a return from the procedure*/
  BasicBlock *exit_true;

  /** pointer to the next basic block, false branch. If
   * null_ptr, the basic block ends with an unconditional jump  */
  BasicBlock *exit_false;

  // This is true if the assembly code relative to this block has already been
  // written
  bool visited;

  std::string label; /**< label of the BB, also will be the label in the
                   generated      code */
  CFG *cfg;          /** < the CFG where this block belongs */
  std::vector<IRInstr> instrs; /** < the instructions themselves. */
  std::string test_var_name;   /** < when generating IR code for an if(expr) or
                             while(expr) etc,     store here the name of the
                             variable     that holds the value of expr */
};

class CFG {
public:
  ~CFG();
  CFG();

  void add_bb(BasicBlock *bb);
  inline std::vector<BasicBlock *> &getBlocks() { return bbs; };

  std::string IR_reg_to_asm(
      std::string reg); /**< helper method: inputs a IR reg or input variable,
                      returns e.g. "-24(%rbp)" for the proper value of 24 */
  void gen_asm_prologue(std::ostream &o);
  void gen_asm(std::ostream &o);
  void gen_asm_epilogue(std::ostream &o);

  std::shared_ptr<Symbol> create_new_tempvar(Type t);
  int get_var_index(std::string name);
  Type get_var_type(std::string name);

  // basic block management
  std::string new_BB_name();
  BasicBlock *current_bb;

  inline void push_table() { symbolTables.push_front(SymbolTable()); }
  void pop_table();

  bool add_symbol(std::string id, Type t, int line);
  std::shared_ptr<Symbol> get_symbol(const std::string &name);

  // Index of the free register with smallest index
  int freeRegister;

protected:
  unsigned int
      nextFreeSymbolIndex; /**< to allocate new symbols in the symbol table */
  int nextBBnumber;        /**< just for naming */

  std::vector<BasicBlock *> bbs; /**< all the basic blocks of this CFG*/

  std::list<SymbolTable> symbolTables;
};
