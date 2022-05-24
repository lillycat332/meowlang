//
//  main.cpp
//  meowlang
//
//  Created by Lilly Cham on 23/05/2022.
//

#include "../../../llvm-project/llvm/examples/Kaleidoscope/include/KaleidoscopeJIT.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Scalar.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace llvm;

// ===== //
// Lexer //
// ===== //

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
  // EOF
  tok_eof = -1,

  // Statements/Instructions
  tok_func = -2,
  tok_return = -3,
  tok_extern = -4,

  // Control flow
  tok_if = -5,
  tok_else = -6,
  tok_for = -7,
  tok_break = -8,

  // Builtin Types
  tok_double = -9,
  tok_int = -10,
  tok_string = -11,
  tok_bool = -12,

  // Structural symbols
  tok_lparen = -13,  // (
  tok_rparen = -14,  // )
  tok_lbrace = -15,  // {
  tok_rbrace = -16,  // }
  tok_rettype = -17, // return type ->
  tok_assign = -18,  // :=

  // Operators
  tok_or = -19,     // ||
  tok_and = -20,    // &&
  tok_not = -21,    //!
  tok_eq = -22,     // ==
  tok_sub = -23,    // -
  tok_mult = -24,   // *
  tok_div = -25,    // /
  tok_mod = -26,    // %
  tok_unary = -27,  // unary operators
  tok_binary = -28, // binary operators

  // Others
  tok_identifier = -29,
  tok_true = -30,
  tok_false = -31,
};

// === //
// AST //
// === //
namespace {
class ExprAST {
public:
  virtual ~ExprAST() = default;
};

class DoubleExprAST : public ExprAST {
  double Val;

public:
  DoubleExprAST(double Val) : Val(Val) {}
};
} // namespace
