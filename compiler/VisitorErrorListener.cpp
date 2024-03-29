#include "VisitorErrorListener.h"

#include "Token.h"
#include "iostream"

using namespace std;

VisitorErrorListener::VisitorErrorListener() : mHasError(false) {}

void VisitorErrorListener::addError(const std::string &message, int line,
                                    ErrorType errorType) {
  switch (errorType) {
  case ErrorType::Error:
    cerr << "Error: ";
    mHasError = true;
    break;
  case ErrorType::Warning:
    cerr << "Warning: ";
    break;
  }

  cerr << "Line " << line << " " << message << endl;
}

void VisitorErrorListener::addError(antlr4::ParserRuleContext *ctx,
                                    const std::string &message,
                                    ErrorType errorType) {
  addError(message, ctx->getStart()->getLine(), errorType);
}

void VisitorErrorListener::addError(const std::string &message,
                                    ErrorType errorType) {
  switch (errorType) {
  case ErrorType::Error:
    cerr << "Error: ";
    mHasError = true;
    break;
  case ErrorType::Warning:
    cerr << "Warning: ";
    break;
  }

  cerr << message << endl;
}
