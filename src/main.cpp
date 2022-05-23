//
//  main.cpp
//  meowlang
//
//  Created by Lilly Cham on 23/05/2022.
//

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

enum Token {
	tok_eof = -1,
	
	// commands
	tok_def = -2,
	tok_extern = -3,
	
	// primary
	tok_identifier = -4,
	tok_number = -5,
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

/// gettok - Return the next token from standard input.
static int gettok() {
	static int LastChar = ' ';
	
	// Skip any whitespace.
	while (isspace(LastChar))
		LastChar = getchar();
	
	if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
		IdentifierStr = LastChar;
		while (isalnum((LastChar = getchar())))
			IdentifierStr += LastChar;
		
		if (IdentifierStr == "func")
			return tok_def;
		if (IdentifierStr == "extern")
			return tok_extern;
		return tok_identifier;
	}
	
	if (isdigit(LastChar) || LastChar == '.') {   // Number: [0-9.]+
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');
		
		NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}
	
	if (LastChar == '#') {
		// Comment until end of line.
		do
			LastChar = getchar();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
		
		if (LastChar != EOF)
			return gettok();
	}
	
	// Check for end of file.  Don't eat the EOF.
	if (LastChar == EOF)
		return tok_eof;
	
	// Otherwise, just return the character as its ascii value.
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}

// CurTok/getNextToken - provide a simple token buffer. CurTok is current token
// the parser is looking at. getNextToken reads another token from the lexer
// and updates CurTok with its results
static int CurTok;
static int getNextToken() {
	return CurTok = gettok();
}

namespace {
// ExprAST - Base expression class for expression nodes
class ExprAST {
public:
	virtual ~ExprAST() = default;
	virtual Value *codegen() = 0;
};

// NumberExprAST - Expression class for numeric literals
class NumberExprAST : public ExprAST {
	double Val;
public:
	NumberExprAST(double Val) : Val(Val) {}
};

// VariableExprAST - Expression class for referencing variables
class VariableExprAST : public ExprAST {
	std::string Name;
	
public:
	VariableExprAST(const std::string &Name) : Name(Name) {}
	Value *codegen() override;
};

// BinaryExprAST - Expression class for binary operator
class BinaryExprAST : public ExprAST {
	char Op;
	std::unique_ptr<ExprAST> LHS, RHS;
	
public:
	BinaryExprAST(char op,
				  std::unique_ptr<ExprAST> LHS,
				  std::unique_ptr<ExprAST> RHS)
	: Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

// CallExprAST - Expression class for function calls
class CallExprAST : public ExprAST {
	std::string	Callee;
	std::vector<std::unique_ptr<ExprAST>> Args;
	
public:
	CallExprAST(const std::string &Callee,
				std::vector<std::unique_ptr<ExprAST>> Args)
	: Callee(Callee), Args(std::move(Args)) {}
};

// PrototypeAST - Expression class which represents the prototype for a function
// which captures its name and argument types (and implicitly the number of
// arguments it takes)
class PrototypeAST {
	std::string Name;
	std::vector<std::string> Args;
	
public:
	PrototypeAST(const std::string &name, std::vector<std::string> Args)
	: Name(name), Args(std::move(Args)) {}
	
	const std::string &getName() const { return Name; }
};

// FunctionAST - Expression class which represents the function itself
class FunctionAST {
	std::unique_ptr<PrototypeAST> Proto;
	std::unique_ptr<ExprAST> Body;
	
public:
	FunctionAST(std::unique_ptr<PrototypeAST> Proto,
				std::unique_ptr<ExprAST> Body)
	: Proto(std::move(Proto)), Body(std::move(Body)) {}
};
}

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

// LogError* - These are helper functions for error handling
std::unique_ptr<ExprAST> LogError(const char *Str) {
	fprintf(stderr, "LogError: %s\n", Str);
	return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
	LogError(Str);
	return nullptr;
}

auto LHS = std::make_unique<VariableExprAST>("x");
auto RHS = std::make_unique<VariableExprAST>("y");
auto Result = std::make_unique<BinaryExprAST>('+', std::move(LHS),
										   std::move(RHS));


// BinOpPrecedence - this holds the precedence for each binary operator that is
// defined.
static std::map<char, int> BinOpPrecedence;

// GetTokPrecedence - get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
	if (!isascii(CurTok))
		return -1;
	int TokPrec = BinOpPrecedence[CurTok];
	if (TokPrec <= 0) return -1;
	return TokPrec;
}

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<PrototypeAST> ParsePrototype();

// definition :: 'func' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
	// Eat func
	getNextToken();
	auto Proto = ParsePrototype();
	if (!Proto) return nullptr;
	
	if (auto E = ParseExpression())
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	return nullptr;
}

// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
	getNextToken();
	return ParsePrototype();
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
	if (auto E = ParseExpression()) {
		// Make an anonymous Proto
		auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}

// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
	auto Result = std::make_unique<NumberExprAST>(NumVal);
	getNextToken(); 										// consume the number
	return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
	getNextToken(); 										// eat (
	auto V = ParseExpression();
	if (!V)
		return nullptr;
	
	if (CurTok != ')')
		return LogError("Expected ')'");
	
	getNextToken(); 										// eat )
	return V;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;
	
	getNextToken(); 										// Eat identifier
	
	if (CurTok != '(')
		return std::make_unique<VariableExprAST>(IdName);
	
	// Call
	getNextToken();											// eat (
	std::vector<std::unique_ptr<ExprAST>> Args;
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
	getNextToken();											// eat )
	return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

// Primary Parser
//		::= identifierexpr
//		::= numberexpr
//		::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
	switch (CurTok) {
		default:
			return LogError("Unknown token when expecting an expression");
		case tok_identifier:
			return ParseIdentifierExpr();
		case tok_number:
			return ParseNumberExpr();
		case '(':
			return ParseParenExpr();
	}
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
																							std::unique_ptr<ExprAST> LHS) {
	// if this is a binop, find precedence.
	while (1) {
		int TokPrec = GetTokPrecedence();
		
		// if this is a binop that binds at least as tightly as the current binop,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return LHS;
		
		// Now we know this is a binop
		int BinOp = CurTok;
		// Eat the binop.
		getNextToken();
		
		// Parse the primary expression after the binary operator
		auto RHS = ParsePrimary();
		if (!RHS)
			return nullptr;
		
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec +1, std::move(RHS));
			if (!RHS)
				return nullptr;
		}
		// Merge the LHS and RHS.
		LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
	}
}

// expression
// 		::= primary BinOpRHS
static std::unique_ptr<ExprAST> ParseExpression() {
	auto LHS = ParsePrimary();
	if (!LHS)
		return nullptr;
	return ParseBinOpRHS(0, std::move(LHS));
}

// Prototype
// 		::= id  '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
	if (CurTok != tok_identifier)
		return LogErrorP("Expected function name in prototype");
	
	std::string FuncName = IdentifierStr;
	getNextToken();
	
	if (CurTok != '(')
		return LogErrorP("Expected '(' in prototype");
	
	std::vector<std::string> ArgNames;
	while (getNextToken() == tok_identifier)
		ArgNames.push_back(IdentifierStr);
	if (CurTok != ')')
		return LogErrorP("Expected ')' in prototype");
	
	// Success, so we can eat the ')'
	getNextToken();
	
	return std::make_unique<PrototypeAST>(FuncName, std::move(ArgNames));
}

static void HandleDefinition() {
	if (ParseDefinition()) {
		fprintf(stderr, "Parsed a function definition.\n");
	} else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleExtern() {
	if (ParseExtern()) {
		fprintf(stderr, "Parsed an extern\n");
	} else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleTopLevelExpression() {
	// Evaluate a top-level expression into an anonymous function.
	if (ParseTopLevelExpr()) {
		fprintf(stderr, "Parsed a top-level expr\n");
	} else {
		// Skip token for error recovery.
		getNextToken();
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
				getNextToken();
				break;
			case tok_def:
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

// Main driver code

int main() {
	// Install standard binary operators.
	// 1 is lowest precedence.
	BinOpPrecedence['<'] = 10;
	BinOpPrecedence['+'] = 20;
	BinOpPrecedence['-'] = 20;
	BinOpPrecedence['*'] = 40; // highest.
	
	// Prime the first token.
	fprintf(stderr, "ready> ");
	getNextToken();
	
	// Run the main "interpreter loop" now.
	MainLoop();
	
	return 0;
}
