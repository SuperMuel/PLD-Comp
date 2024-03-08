#pragma once

#include "ParserRuleContext.h"

enum class ErrorType { Error, Warning };

class VisitorErrorListener {
public:
  VisitorErrorListener();
  inline bool hasError() const { return mHasError; }
  virtual void addError(antlr4::ParserRuleContext *ctx,
                        const std::string &message,
                        ErrorType errorType = ErrorType::Error);
  virtual void addError(const std::string &message,
                        ErrorType errorType = ErrorType::Error);

protected:
  bool mHasError;
};
