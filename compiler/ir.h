#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Symbol.h"
#include "Type.h"

class BasicBlock;
class CFG;

const std::string registers[] = {"r8d",  "r9d",  "r10d", "r11d",
                                 "r12d", "r13d", "r14d", "r15d"};

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
    ret,
  } Operation;

  /**  constructor */
  IRInstr(BasicBlock *bb_, Operation op, Type t,
          const std::vector<std::string> &params);

  void genAsm(std::ostream &os, CFG *cfg);

  friend std::ostream &operator<<(std::ostream &os, IRInstr &instruction);

private:
  Type outType;
  std::vector<std::string> params;
  Operation op;
  BasicBlock *block;
};

class BasicBlock {
public:
  BasicBlock(CFG *cfg, std::string entry_label);
  void gen_asm(std::ostream &
                   o); /**< x86 assembly code generation for this
                                                    basic block (very simple) */
  std::string add_IRInstr(IRInstr::Operation op, Type t,
                          std::vector<std::string> params, CFG *cfg);

  // No encapsulation whatsoever here. Feel free to do better.
  BasicBlock *exit_true;  /**< pointer to the next basic block, true branch. If
                             nullptr, return from procedure */
  BasicBlock *exit_false; /**< pointer to the next basic block, false branch. If
                             null_ptr, the basic block ends with an
                             unconditional jump */
  std::string label;      /**< label of the BB, also will be the label in the
                        generated      code */
  CFG *cfg;               /** < the CFG where this block belongs */
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

  std::string IR_reg_to_asm(
      std::string reg); /**< helper method: inputs a IR reg or input variable,
                      returns e.g. "-24(%rbp)" for the proper value of 24 */
  void gen_asm_prologue(std::ostream &o);
  void gen_asm(std::ostream &o);
  void gen_asm_epilogue(std::ostream &o);

  // symbol table methods
  void add_to_symbol_table(std::string name, Type t, int line);
  std::string create_new_tempvar(Type t);
  int get_var_index(std::string name);
  Type get_var_type(std::string name);

  // basic block management
  std::string new_BB_name();
  BasicBlock *current_bb;

  std::map<std::string, Symbol *> symbolTable;

  // Index of the free register with smallest index
  int freeRegister;

protected:
  int nextFreeSymbolIndex; /**< to allocate new symbols in the symbol table */
  int nextBBnumber;        /**< just for naming */

  std::vector<BasicBlock *> bbs; /**< all the basic blocks of this CFG*/
};
