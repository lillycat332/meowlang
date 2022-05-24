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

/// The lexer returns tokens [0-255] if it is an unknown character, otherwise
/// one of these for known things.
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
  tok_unary = -19,  // unary operators
  tok_binary = -20, // binary operators

  // Others
  tok_identifier = -21,
  tok_true = -22,
  tok_false = -23,
};

// === //
// AST //
// === //
namespace {
/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() = default;
};

/// Types
/// DoubleExprAST - Expression class for double literals like "12.3".
class DoubleExprAST : public ExprAST {
  double Val;

public:
  DoubleExprAST(double Val) : Val(Val) {}
};

/// IntegerExprAST - Expression class for numeric literals like "12".
class IntegerExprAST : public ExprAST {
  int Val;

public:
  IntegerExprAST(int Val) : Val(Val) {}
};

/// BooleanExprAST - Expression class for a boolean literal.
class BooleanExprAST : public ExprAST {
  bool Val;

public:
  BooleanExprAST(bool Val) : Val(Val) {}
};

/// StringExprAST - Expression class for string literals.
class StringExprAST : public ExprAST {
  std::string Val;

public:
  StringExprAST(std::string Val) : Val(Val) {}
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
      : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(Callee), Args(std::move(Args)) {}
};

} // namespace
