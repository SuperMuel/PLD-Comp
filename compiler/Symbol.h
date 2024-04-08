/*#pragma once
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

  Symbol(Type type, const std::string &lexeme)
      : type(type), lexeme(lexeme), used(false), offset(0), line(1) {}
  Symbol(Type type, const std::string &lexeme, int line)
      : type(type), lexeme(lexeme), used(false), offset(0), line(line) {}
};*/

#pragma once
#include "Type.h"
#include <string>

struct Symbol {
  // Memory offset (in bytes)
  int offset;
  // Whether or not this variable has been used
  bool used;
  // In which line the symbol was declared
  int line;
  // The type of the symbol
  Type type;
  // The lexeme corresponding to this symbol
  std::string lexeme;
  // Size of the array (0 for non-array variables)
  int arraySize = 0;

  // Constructor for non-array variables
  Symbol(Type type, const std::string &lexeme)
      : type(type), lexeme(lexeme), used(false), offset(0), line(1), arraySize(0)  {}

  Symbol(Type type, const std::string &lexeme, int line)
      : type(type), lexeme(lexeme), used(false), offset(0), line(line), arraySize(0)  {}

  // Constructor for array variables
  Symbol(Type type, const std::string &lexeme, int line, int arraySize)
      : type(type), lexeme(lexeme), used(false), offset(0), line(line), arraySize(arraySize) {}
};
