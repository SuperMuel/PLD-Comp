#include "Type.h"

unsigned int getSize(Type t) {
  switch (t) {
  case Type::INT:
    return 4;
  case Type::CHAR:
    return 1;
  }
  return 0;
}
