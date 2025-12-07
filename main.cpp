#include<iostream>
#include<memory>
#include<utility>
#include<vector>
#include<map>

//=== Lexer
enum Token {
    tok_eof = -1,

    // commands 
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5
};

static std::string IdentifierStr;
static double NumVal;

static int gettok(){
    static int LastChar = ' ';
    
    // skip white
    while(isspace(LastChar)) LastChar = getchar();

    // keyword & identifier
    if(isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while(isalnum(LastChar = getchar())) IdentifierStr += LastChar;

        // keyword
        if(IdentifierStr == "def") return tok_def;
        if(IdentifierStr == "extern") return tok_extern;

        // identifier
        return tok_identifier;
    }

    // number
    if(isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do{
            NumStr += LastChar;
            LastChar = getchar();
        } while(isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(),0);
        return tok_number;
    }

    // comment
    if(LastChar == '#') {
        do LastChar = getchar();
        while(LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if(LastChar != EOF) return gettok();
    }

    // end
    if(LastChar == EOF) return tok_eof;

    // other character
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

//=== AST
class ExprAST {
public:
    virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double Val):Val(Val){}
};

class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(const std::string& Name):Name(Name){}
};

class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS,RHS;

public:
    BinaryExprAST(char Op,std::unique_ptr<ExprAST> LHS,std::unique_ptr<ExprAST> RHS):
        Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST(const std::string& Callee,std::vector<std::unique_ptr<ExprAST>> Args):
        Callee(Callee), Args(std::move(Args)){}
};

class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(const std::string& Name, std::vector<std::string> Args):
        Name(Name), Args(std::move(Args)){}
};

class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body):
        Proto(std::move(Proto)), Body(std::move(Body)){}
};

//=== Parser
static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

static std::map<char,int> BinopPrecedence;
static int GetTokPrecedence() {
    if(!isascii(CurTok)) return -1;

    int TokPrec = BinopPrecedence[CurTok];
    if(TokPrec <= 0) return -1;

    return TokPrec;
}

std::unique_ptr<ExprAST> LogError(const char* Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char* Str) {
    LogError(Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken();
    auto V = ParseExpression();
    if(!V) return nullptr;

    if(CurTok != ')') return LogError("expected ')'");
    getNextToken();
    return V;
}

// identifierexpr 
//  ::= identifier
//  ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;
    getNextToken();

    //Variable
    if(CurTok != '(') return std::make_unique<VariableExprAST>(IdName);

    // Call
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> Args;
    if(CurTok != ')') {
        while(true) {
            if(auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else return nullptr;

            if(CurTok == ')') break;

            if(CurTok != ',') return LogError("Expected ')' or ',' in argument list");

            getNextToken();
        }
    }

    getNextToken();
    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

// primary
//  ::= identifier
//  ::= numberexpr
//  ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch(CurTok) {
        default: return LogError("unkonwn token when expecting an expression");
        case tok_identifier: return ParseIdentifierExpr();
        case tok_number: return ParseNumberExpr();
        case '(': return ParseParenExpr();
    }
}

// binoprhs ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
    while(true) {
        int TokPrec = GetTokPrecedence();
        if(TokPrec < ExprPrec) return LHS;

        int BinOp = CurTok;
        getNextToken();

        auto RHS = ParsePrimary();
        if(!RHS) return nullptr;

        int NextPrec = GetTokPrecedence();
        if(TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec+1,std::move(RHS));
            if(!RHS) return nullptr;
        }

        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

// expression ::= primary binoprhs
static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if(!LHS) return nullptr;

    return ParseBinOpRHS(0,std::move(LHS));
}

// prototype ::= id '('id*')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if(CurTok != tok_identifier) return LogErrorP("Expected function name in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();
    if(CurTok != '(') return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;
    while(getNextToken() == tok_identifier) ArgNames.push_back(IdentifierStr);
    if(CurTok != ')') return LogErrorP("Expected ')' in prototype");

    getNextToken();

    return std::make_unique<PrototypeAST>(FnName,std::move(ArgNames));
}

// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken();
    auto Proto = ParsePrototype();
    if(!Proto) return nullptr;

    if(auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto),std::move(E));
    return nullptr;
}

// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if(auto E = ParseExpression()) {
        auto Proto = std::make_unique<PrototypeAST>("__anon_expr",std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto),std::move(E));
    }
    return nullptr;
}

// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken();
    return ParsePrototype();
}

//=== TopLevel
static void HandleDefinition() {
    if(ParseDefinition())
        fprintf(stderr,"Parsed a function definition.\n");
    else getNextToken();
}

static void HandleExtern() {
    if(ParseExtern())
        fprintf(stderr,"Parsed an extern.\n");
    else getNextToken();
}

static void HandleTopLevelExpression() {
    if(ParseTopLevelExpr())
        fprintf(stderr,"Parsed a top-level expr.\n");
    else getNextToken();
}

// top ::= definition | external | expression | ';'
static void MainLoop() {
    while(true) {
        fprintf(stderr,"ready> ");
        switch(CurTok) {
            case tok_eof: return;
            case ';': getNextToken(); break;
            case tok_def: HandleDefinition(); break;
            case tok_extern: HandleExtern(); break;
            default: HandleTopLevelExpression(); break;
        }
    }
}

//=== Driver
int main()
{
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    fprintf(stderr,"ready> ");
    getNextToken();

    MainLoop();

    return 0;
}

