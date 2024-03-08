#include "VisitorErrorListener.h"

#include "Token.h"
#include "iostream"

using namespace std;

VisitorErrorListener::VisitorErrorListener() : mHasError(false) {}

void VisitorErrorListener::addError(antlr4::ParserRuleContext *ctx,
                                    const std::string &message,
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

  int line = ctx->getStart()->getLine();
  cerr << "Line " << line << " " << message << endl;
}

void VisitorErrorListener::addError(const std::string &message,
                                    ErrorType errorType) {
  mHasError = true;
  switch (errorType) {
  case ErrorType::Error:
    cerr << "Error: ";
    break;
  case ErrorType::Warning:
    cerr << "Warning: ";
    break;
  }

  cerr << message << endl;
}
