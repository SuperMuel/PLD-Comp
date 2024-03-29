#pragma once
#include "Type.h"
#include <string>
struct Symbol {
  // Memory offset (in bytes)
  int offset;
  // Wheter or not this variable has been used
  bool used;
  // In which line the symbol was declared
  int line;
  // The type of the symbol
  Type type;
  // The lexeme corresponding to this symbol
  std::string lexeme;

  Symbol(const std::string &lexeme)
      : lexeme(lexeme), used(false), offset(0), line(1) {}
  Symbol(const std::string &lexeme, int line)
      : lexeme(lexeme), used(false), offset(0), line(line) {}
};
