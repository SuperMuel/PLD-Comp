#pragma once
#include "Type.h"
struct Symbol {
  // Memory offset (in bytes)
  int offset;
  // Wheter or not this variable has been used
  bool used;
  // In which line the symbol was declared
  int line;
  // The type of the symbol
  Type type;

  Symbol() : used(false), offset(0), line(1) {}
  Symbol(int line) : used(false), offset(0), line(line) {}
};
