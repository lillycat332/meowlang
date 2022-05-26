//
//  main.cpp
//  meowlang
//
//  Created by Lilly Cham on 23/05/2022.
//

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
#include <stdio.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace llvm;
namespace meowlang {

  ///======== ///
  /// Globals ///
  ///======== ///

  static std::string IdentifierStr; // filled in if tok_identifier
  static double DoubleVal;          // Filled in if tok_double
  static int IntVal;                // Filled in if tok_int
  static bool BoolVal;              // Filled in if tok_bool
  static std::string StrVal;        // Filled in if tok_string

  namespace lexer {

    // ===== //
    // Lexer //
    // ===== //

    /// The lexer returns tokens [0-255] if it is an unknown character,
    /// otherwise one of these for known things.
    enum Token {
      // EOF
      tok_eof = -1,

      // Statements/Instructions
      tok_func = -2,
      tok_return = -3,
      tok_extern = -4,
      tok_var = -5,
      tok_let = -6,

      // Control flow
      tok_if = -7,
      tok_else = -8,
      tok_for = -9,
      tok_break = -10,

      // Builtin Types
      tok_double = -11,
      tok_int = -12,
      tok_string = -13,
      tok_bool = -14,

      // Structural symbols
      tok_lparen = -17,  // (
      tok_rparen = -18,  // )
      tok_lbrace = -19,  // {
      tok_rbrace = -20,  // }
      tok_rettype = -21, // return type ->
      tok_assign = -22,  // :=

      // Operators
      tok_unary = -23,  // unary operators
      tok_binary = -24, // binary operators

      // Others
      tok_identifier = -25,
      tok_true = -26,
      tok_false = -27,
    };

    // gettok - returns the next token from standard input/the file.
    static int gettok() {
      static char16_t LastChar = ' ';

      // skip over whitespace
      while (isspace(LastChar))
        LastChar = getchar();

      if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
          IdentifierStr += LastChar;

        // check for keywords
        if (IdentifierStr == "func")
          return tok_func;

        if (IdentifierStr == "extern")
          return tok_extern;

        if (IdentifierStr == "return")
          return tok_return;

        if (IdentifierStr == "var")
          return tok_var;

        if (IdentifierStr == "let")
          return tok_let;
      }

      // Check for special symbols / syntactic sugar (λ)
      if (LastChar == u'λ') // lambda, which is effectively syntactic sugar for
        return tok_func;    // function definition

      // Check for doubles, the default number type in Meowlang
      if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
        std::string DoubleStr;
        do {
          DoubleStr += LastChar;
          LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        DoubleVal = strtod(DoubleStr.c_str(), nullptr);
        return tok_double;
      }

      // Check for comments.
      if (LastChar == '#') {
        do {
          LastChar = getchar();
        } while (LastChar != std::char_traits<char16_t>::eof() &&
                 LastChar != '\n' && LastChar != '\r');

        if (LastChar != std::char_traits<char16_t>::eof())
          return gettok();
      }

      // Check for EOF, but don't eat the EOF.
      if (LastChar == std::char_traits<char16_t>::eof())
        return tok_eof;

      // Otherwise, just return the character as its ascii value.
      int ThisChar = LastChar;
      LastChar = getchar();
      return ThisChar;
    }

  } // namespace lexer

  namespace AST {

    // === //
    // AST //
    // === //

    /// ExprAST - Base class for all expression nodes.
    class ExprAST {
    public:
      virtual ~ExprAST() = default;
      virtual Value *codegen() = 0;
    };

    // ===== //
    // Types //
    // ===== //

    /// DoubleExprAST - Expression class for double literals like "12.3".
    class DoubleExprAST : public ExprAST {
      double Val;

    public:
      DoubleExprAST(double Val) : Val(Val) {}
      Value *codegen() override;
    };

    /// IntegerExprAST - Expression class for integer literals like "12".
    class IntegerExprAST : public ExprAST {
      int Val;

    public:
      IntegerExprAST(int Val) : Val(Val) {}
      Value *codegen() override;
    };

    /// BooleanExprAST - Expression class for a boolean literal like true.
    class BooleanExprAST : public ExprAST {
      bool Val;

    public:
      BooleanExprAST(bool Val) : Val(Val) {}
      Value *codegen() override;
    };

    /// StringExprAST - Expression class for string literals
    /// like "hello world".
    class StringExprAST : public ExprAST {
      std::string Val;

    public:
      StringExprAST(std::string Val) : Val(Val) {}
      Value *codegen() override;
    };

    /// VariableExprAST - Expression class for referencing a variable, like "a".
    class VariableExprAST : public ExprAST {
      std::string Name;

    public:
      VariableExprAST(const std::string &Name) : Name(Name) {}
      Value *codegen() override;
    };

    /// BinaryExprAST - Expression class for a binary operator.
    class BinaryExprAST : public ExprAST {
      char16_t Op;
      std::unique_ptr<ExprAST> LHS, RHS;

    public:
      BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                    std::unique_ptr<ExprAST> RHS)
          : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
      Value *codegen() override;
    };

    /// CallExprAST - Expression class for function calls.
    class CallExprAST : public ExprAST {
      std::string Callee;
      std::vector<std::unique_ptr<ExprAST>> Args;

    public:
      CallExprAST(const std::string &Callee,
                  std::vector<std::unique_ptr<ExprAST>> Args)
          : Callee(Callee), Args(std::move(Args)) {}
      Value *codegen() override;
    };

    /// PrototypeAST - This class represents the "prototype" for a function,
    /// which captures its name, and its argument names (thus implicitly the
    /// number of arguments the function takes).
    class PrototypeAST {
      std::string Name;
      std::vector<std::string> Args;

    public:
      PrototypeAST(const std::string &name, std::vector<std::string> Args)
          : Name(name), Args(std::move(Args)) {}

      const std::string &getName() const { return Name; }
    };

    /// FunctionAST - This class represents a function definition itself.
    class FunctionAST {
      std::unique_ptr<PrototypeAST> Proto;
      std::unique_ptr<ExprAST> Body;

    public:
      FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                  std::unique_ptr<ExprAST> Body)
          : Proto(std::move(Proto)), Body(std::move(Body)) {}
    };

  } // namespace AST

  namespace parse {

    // ====== //
    // Parser //
    // ====== //

    static std::unique_ptr<AST::ExprAST> ParseExpression();

    /// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the
    /// current token the parser is looking at.  getNextToken reads another
    /// token from the lexer and updates CurTok with its results.
    static int CurTok;
    static int getNextToken() { return CurTok = lexer::gettok(); }

    /// BinopPrecedence - holds the precedence for each binary operator defined.
    static std::map<char, int> BinopPrecedence;

    /// GetTokPrecedence - get precedence of the pending binary operator token.
    static int GetTokPrecedence() {
      if (!isascii(CurTok))
        return -1;

      // make sure it's a declared binop.
      int TokPrec = BinopPrecedence[CurTok];
      if (TokPrec <= 0)
        return -1;
      return TokPrec;
    }

    /// LogError* - These are helper functions for error handling.
    /// They allow us to return errors from the parser.
    std::unique_ptr<AST::ExprAST> LogError(const char *Str) {
      fprintf(stderr, "LogError: %s\n", Str);
      return nullptr;
    }

    std::unique_ptr<AST::PrototypeAST> LogErrorP(const char *Str) {
      LogError(Str);
      return nullptr;
    }

    // Parser implementation

    /// doubleexpr ::= double
    static std::unique_ptr<AST::ExprAST> ParseDoubleExpr() {
      auto Result = std::make_unique<AST::DoubleExprAST>(DoubleVal);
      // eat the value
      getNextToken();
      return std::move(Result);
    }

    /// intexpr ::= int
    static std::unique_ptr<AST::ExprAST> ParseIntegerExpr() {
      auto Result = std::make_unique<AST::IntegerExprAST>(IntVal);
      // eat the value
      getNextToken();
      return std::move(Result);
    }

    /// boolexpr ::= bool
    static std::unique_ptr<AST::ExprAST> ParseBooleanExpr() {
      auto Result = std::make_unique<AST::BooleanExprAST>(BoolVal);
      // eat the value
      getNextToken();
      return std::move(Result);
    }

    /// stringexpr ::= string
    static std::unique_ptr<AST::ExprAST> ParseStringExpr() {
      auto Result = std::make_unique<AST::StringExprAST>(StrVal);
      // eat the value
      getNextToken();
      return std::move(Result);
    }

    /// parenexpr ::= '(' expression ')'
    static std::unique_ptr<AST::ExprAST> ParseParenExpr() {
      // eat (.
      getNextToken();
      auto V = ParseExpression();
      if (!V)
        return nullptr;

      if (CurTok != ')')
        return LogError("expected ')'");
      // eat ).
      getNextToken();
      return V;
    }

    /// braceexpr ::= '{' expression '}'
    static std::unique_ptr<AST::ExprAST> ParseBraceExpr() {
      // eat {.
      getNextToken();
      auto V = ParseExpression();
      if (!V)
        return nullptr;

      if (CurTok != '}')
        return LogError("expected '}'");
      // eat }.
      getNextToken();
      return V;
    }

    /// identifierexpr
    ///   ::= identifier
    ///   ::= identifier '(' expression* ')'
    static std::unique_ptr<AST::ExprAST> ParseIdentifierExpr() {
      std::string IdName = IdentifierStr;

      // eat identifier.
      getNextToken();

      // variable ref
      if (CurTok != '(')
        return std::make_unique<AST::VariableExprAST>(IdName);

      // Call.
      getNextToken(); // eat (
      std::vector<std::unique_ptr<AST::ExprAST>> Args;
      if (CurTok != ')') {
        while (1) {
          if (auto Arg = ParseExpression())
            Args.push_back(std::move(Arg));
          else
            return nullptr;

          if (CurTok == ')')
            break;

          if (CurTok != ',')
            return LogError("Expected ')' or ',' in argument list");
          getNextToken();
        }
      }

      // Eat the ')'.
      getNextToken();

      return std::make_unique<AST::CallExprAST>(IdName, std::move(Args));
    }

    /// prototype
    ///   ::= id '(' id* ')'
    static std::unique_ptr<AST::PrototypeAST> ParsePrototype() {
      if (CurTok != lexer::tok_identifier)
        return LogErrorP("Expected function name in prototype");

      std::string FnName = IdentifierStr;
      getNextToken();

      if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

      // read list of argument names.
      std::vector<std::string> ArgNames;
      while (getNextToken() == lexer::tok_identifier)
        ArgNames.push_back(IdentifierStr);
      if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

      // success. eat ')'.
      getNextToken();

      return std::make_unique<AST::PrototypeAST>(FnName, std::move(ArgNames));
    }

    /// primary
    ///   ::= identifierexpr
    ///   ::= numberexpr
    ///   ::= parenexpr
    static std::unique_ptr<AST::ExprAST> ParsePrimary() {
      switch (CurTok) {
      default:
        return LogError("unknown token when expecting an expression");
      case lexer::tok_identifier:
        return ParseIdentifierExpr();
      case lexer::tok_double:
        return ParseDoubleExpr();
      case lexer::tok_int:
        return ParseIntegerExpr();
      case lexer::tok_bool:
        return ParseBooleanExpr();
      case lexer::tok_string:
        return ParseStringExpr();
      case '{':
        return ParseBraceExpr();
      case '(':
        return ParseParenExpr();
      }
    }

    /// binoprhs
    ///   ::= ('+' primary)*
    static std::unique_ptr<AST::ExprAST>
    ParseBinOpRHS(int ExprPrec, std::unique_ptr<AST::ExprAST> LHS) {
      // if it's a binop, find precedence.
      while (true) {
        int TokPrec = GetTokPrecedence();

        // if it's is a binop that binds at least as tightly as the current
        // binop, eat it, otherwise we are done.
        if (TokPrec < ExprPrec)
          return LHS;

        // now we know this is a binop.
        int BinOp = CurTok;
        getNextToken();

        // parse the primary expression after the binary operator.
        auto RHS = ParsePrimary();
        if (!RHS)
          return nullptr;

        // if BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
          RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
          if (!RHS)
            return nullptr;
        }

        // merge LHS/RHS.
        LHS = std::make_unique<AST::BinaryExprAST>(BinOp, std::move(LHS),
                                                   std::move(RHS));
      }
    }

    /// expression
    ///   ::= primary binoprhs
    ///
    static std::unique_ptr<AST::ExprAST> ParseExpression() {
      auto LHS = ParsePrimary();
      if (!LHS)
        return nullptr;

      return ParseBinOpRHS(0, std::move(LHS));
    }

    /// definition ::= 'def' prototype expression
    static std::unique_ptr<AST::FunctionAST> ParseDefinition() {
      getNextToken(); // eat def.
      auto Proto = ParsePrototype();
      if (!Proto)
        return nullptr;

      if (auto E = ParseExpression())
        return std::make_unique<AST::FunctionAST>(std::move(Proto),
                                                  std::move(E));
      return nullptr;
    }

    /// toplevelexpr ::= expression
    static std::unique_ptr<AST::FunctionAST> ParseTopLevelExpr() {
      if (auto E = ParseExpression()) {
        // Make an anonymous proto.
        auto Proto = std::make_unique<AST::PrototypeAST>(
            "__anon_expr", std::vector<std::string>());
        return std::make_unique<AST::FunctionAST>(std::move(Proto),
                                                  std::move(E));
      }
      return nullptr;
    }

    /// external ::= 'extern' prototype
    static std::unique_ptr<AST::PrototypeAST> ParseExtern() {
      getNextToken(); // eat extern.
      return ParsePrototype();
    }
  } // namespace parse

  /// =============== ///
  /// Code Generation ///
  /// =============== ///

  static LLVMContext TheContext;
  static IRBuilder<> Builder(TheContext);
  static std::unique_ptr<Module> TheModule;
  static std::map<std::string, Value *> NamedValues;

  Value *LogErrorV(const char *Str) {
    parse::LogError(Str);
    return nullptr;
  }

  Value *AST::DoubleExprAST::codegen() {
    return ConstantFP::get(TheContext, APFloat(Val));
  }

  Value *AST::IntegerExprAST::codegen() {
    return ConstantInt::get(TheContext, APInt(64, Val));
  }

  Value *AST::BooleanExprAST::codegen() {
    return ConstantInt::get(TheContext, APInt(1, Val));
  }

  Value *AST::StringExprAST::codegen() {
    return ConstantDataArray::getString(TheContext, Val, true);
  }

  Value *AST::VariableExprAST::codegen() {
    // Look this variable up in the function.
    Value *V = NamedValues[Name];
    if (!V)
      return LogErrorV("Unknown variable name");
    return V;
  }

  Value *AST::BinaryExprAST::codegen() {
    Value *L = LHS->codegen();
    Value *R = LHS->codegen();
    if (!L || !R)
      return nullptr;
    switch (Op) {
    case '+':
      return Builder.CreateFAdd(L, R, "addtmp");

    case '-':
      return Builder.CreateFSub(L, R, "subtmp");

    case u'×':
    case '*':
      return Builder.CreateFMul(L, R, "multmp");

    case u'÷':
    case '/':
      return Builder.CreateFDiv(L, R, "divtmp");

    case '%':
      return Builder.CreateFRem(L, R, "modtmp");

    case '<':
      L = Builder.CreateFCmpULT(L, R, "cmptmp");
      // Convert bool 0/1 to double 0.0 or 1.0.
      return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");

    default:
      return LogErrorV("invalid binary operator");
    }
  }

  Value *AST::CallExprAST::codegen() {
    // lookup the name in the module table
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
      return LogErrorV("Unknown function referenced");

    // argument mismatch error
    if (CalleeF->arg_size() != Args.size())
      return LogErrorV("Incorrect # arguments passed");

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
      ArgsV.push_back(Args[i]->codegen());
      if (!ArgsV.back())
        return nullptr;
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
  }

  static void HandleDefinition() {
    if (ParseDefinition()) {
      fprintf(stderr, "Parsed a function definition.\n");
    } else {
      // Skip token for error recovery.
      parse::getNextToken();
    }
  }

  static void HandleExtern() {
    if (ParseExtern()) {
      fprintf(stderr, "Parsed an extern\n");
    } else {
      // Skip token for error recovery.
      parse::getNextToken();
    }
  }

  static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (parse::ParseTopLevelExpr()) {
      fprintf(stderr, "Parsed a top-level expr\n");
    } else {
      // Skip token for error recovery.
      parse::getNextToken();
    }
  }

  /// top ::= definition | external | expression | ';'
  static void MainLoop() {
    while (true) {
      fprintf(stderr, "ready> ");
      switch (CurTok) {
      case tok_eof:
        return;
      case ';': // ignore top-level semicolons.
        parse::getNextToken();
        break;
      case tok_func:
        HandleDefinition();
        break;
      case tok_extern:
        HandleExtern();
        break;
      default:
        HandleTopLevelExpression();
        break;
      }
    }
  }
} // namespace meowlang
int main() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  parse::BinopPrecedence['<'] = 10;
  parse::BinopPrecedence['+'] = 20;
  parse::BinopPrecedence['-'] = 20;
  parse::BinopPrecedence['*'] = 40;
  parse::BinopPrecedence['/'] = 50; // highest.

  // Prime the first token.
  fprintf(stderr, "ready> ");
  parse::getNextToken();

  MainLoop();

  return 0;
}