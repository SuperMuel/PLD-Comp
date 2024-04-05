#pragma once

#include "ParserRuleContext.h"

enum class ErrorType { Error, Warning };

class VisitorErrorListener {
public:
  static inline bool hasError() { return mHasError; }
  static void addError(antlr4::ParserRuleContext *ctx,
                       const std::string &message,
                       ErrorType errorType = ErrorType::Error);
  static void addError(const std::string &message, int line,
                       ErrorType errorType = ErrorType::Error);
  static void addError(const std::string &message,
                       ErrorType errorType = ErrorType::Error);

protected:
  static bool mHasError;
};
