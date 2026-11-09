# C-- Compiler — Design Report (input for AI-generated PDF)

You are given a complete working C-- compiler front-end (lexer + recursive-descent
parser + three-address-code generator) and a sample program with its actual output.
Produce a polished, print-ready **PDF report** of this project.

## Task brief for the PDF

The report should be a self-contained design document. Suggested structure:

1. **Cover / title page** — project: "C-- Compiler — Design Report".
   Include: language (C-- subset), implementation language (C++17), build tooling
   (make, g++ `-Wall -Wextra -O2`; debug build `-g -O0`).
2. **Table of contents.**
3. **Overview & language supported** — int/char/void, globals, functions with
   parameters, assignment, if/else, while, in/out, return, arithmetic (+ - * / %),
   relational operators, integer and character constants. Note what is NOT
   supported (see README bullets).
4. **Architecture** — `Lexer` → `Parser` (recursive descent) → `CodeGen`;
   one-token lookahead; TAC emitted bottom-up; symbol table built during parsing.
5. **Lexical analysis** — describe the token classes, lookahead handling for
   `<=`/`>=`/`==`/`!=`, error-token strategy (TOK_ERROR), line tracking.
6. **Syntax analysis** — precedence-tiered recursive descent
   (parseExpression → additive → term → factor), statement dispatch,
   if/while lowering to conditional/unconditional `goto`s, labels `L1..`, temps `t1..`.
7. **TAC generation** — what emits, what doesn't (declarations emit nothing);
   show the mapping from source constructs to emitted triples; walk through
   `examples/hello.cmm` instruction by instruction using the attached `tac.txt`.
8. **Symbol table** — columns (Name, Kind, Type, Address, Initial), address
   allocation (starts at 1000, +4), flat (unscoped) table, temporaries excluded,
   function names included; show `symtab.txt` and explain each row.
   Note the `Initial` column is always `-` (the C-- grammar has no declaration
   initializers; this is a phase limitation, not a bug).
9. **Verification** — build log, successful run on `hello.cmm`, the two error
   examples and their messages, exit codes. (You may state the commands:
   `make`, `./build/cmmc examples/hello.cmm`.)
10. **Design decisions & trade-offs** — no AST/intermediate IR other than TAC;
    single pass; no type checking; naive TAC (e.g., `sum = t1` copies) —
    explain why this is intentional for a teaching front-end.
11. **Appendix: full source listings** — the complete code, in the order below.

Formatting requirements: clear sans-serif headings, code in monospace with
syntax highlighting, page numbers, 1in margins, approximately the length a
typical compiler-course project report takes. Do not invent features that are
not in the code; describe only what exists. If anything is ambiguous, mark it
as a design note rather than fabricating details.

---
## 1 · Sample input program (`examples/hello.cmm`)

<!-- file: examples/hello.cmm -->

```c
int add(int a, int b) {
    int sum;
    sum = a + b;
    return sum;
}

int main() {
    int x, y, result;
    char grade;

    in(x);
    in(y);

    if (x >= y) {
        result = x - y;
    } else {
        result = y - x;
    }

    while (x != 0) {
        x = x - 1;
    }

    grade = 'A';

    out(result);
    out(grade);

    return 0;
}
```

## 2 · Compiler source code (`src/`)

### 2.1 `src/main.cpp`

```cpp
#include "parser.hpp"

#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <source-file.cmm>\n";
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cerr << "Error: cannot open file " << argv[1] << "\n";
        return 1;
    }

    try {
        Parser parser(file);
        parser.parse();
        parser.codegen().writeOutputs("tac.txt", "symtab.txt");
        cout << "Parsing successful.\n"
             << "TAC written to tac.txt\n"
             << "Symbol table written to symtab.txt\n";
    } catch (const ParseError& error) {
        cerr << error.what() << '\n';
        return 1;
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
```

### 2.2 `src/lexer.hpp`

```cpp
#ifndef CMM_LEXER_HPP
#define CMM_LEXER_HPP

#include <istream>
#include <string>

using namespace std;

enum TokenType {
    TOK_KW_INT, TOK_KW_CHAR, TOK_KW_IF, TOK_KW_ELSE, TOK_KW_WHILE,
    TOK_KW_IN, TOK_KW_OUT, TOK_KW_RETURN, TOK_KW_VOID,
    TOK_ID, TOK_NUM, TOK_CHARCONST,
    TOK_ASSIGN, TOK_LT, TOK_GT, TOK_LE, TOK_GE, TOK_EQ, TOK_NE,
    TOK_PLUS, TOK_MINUS, TOK_TIMES, TOK_DIV, TOK_MOD,
    TOK_SEMI, TOK_COMMA, TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_EOF, TOK_ERROR
};

const char* tokenName(TokenType type);

struct Token {
    TokenType type;
    string lexeme;
    int line;
};

class Lexer {
public:
    explicit Lexer(istream& input);
    Token getNextToken();

private:
    istream& in;
    int line;

    Token makeToken(TokenType type, const string& lexeme);
    Token lookupIdOrKeyword(const string& lexeme);
    TokenType singleCharType(char c);
};

#endif
```

### 2.3 `src/lexer.cpp`

```cpp
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <cctype>
#include "lexer.hpp"

using namespace std;

const char* tokenName(TokenType t) {
    switch (t) {
        case TOK_KW_INT: return "KW_INT";
        case TOK_KW_CHAR: return "KW_CHAR";
        case TOK_KW_IF: return "KW_IF";
        case TOK_KW_ELSE: return "KW_ELSE";
        case TOK_KW_WHILE: return "KW_WHILE";
        case TOK_KW_IN: return "KW_IN";
        case TOK_KW_OUT: return "KW_OUT";
        case TOK_KW_RETURN: return "KW_RETURN";
        case TOK_KW_VOID: return "KW_VOID";
        case TOK_ID: return "ID";
        case TOK_NUM: return "NUM";
        case TOK_CHARCONST: return "CHARCONST";
        case TOK_ASSIGN: return "ASSIGN";
        case TOK_LT: return "LT";
        case TOK_GT: return "GT";
        case TOK_LE: return "LE";
        case TOK_GE: return "GE";
        case TOK_EQ: return "EQ";
        case TOK_NE: return "NE";
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_TIMES: return "TIMES";
        case TOK_DIV: return "DIV";
        case TOK_MOD: return "MOD";
        case TOK_SEMI: return "SEMI";
        case TOK_COMMA: return "COMMA";
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

static map<string, TokenType> keywordTable = {
    {"int", TOK_KW_INT}, {"char", TOK_KW_CHAR}, {"if", TOK_KW_IF},
    {"else", TOK_KW_ELSE}, {"while", TOK_KW_WHILE}, {"in", TOK_KW_IN},
    {"out", TOK_KW_OUT}, {"return", TOK_KW_RETURN}, {"void", TOK_KW_VOID}
};

enum State {
    S0_START,
    S1_IN_ID,
    S2_IN_NUM,
    S3_LT_SEEN,
    S4_LE_DONE,      
    S5_LT_DONE,      
    S6_GT_SEEN,
    S7_GE_DONE,      
    S8_GT_DONE,      
    S9_ASSIGN_SEEN,
    S10_EQ_DONE,     
    S11_ASSIGN_DONE, 
    S12_NOT_SEEN,
    S13_NE_DONE,     
    S14_CHAR_OPEN,
    S15_CHAR_BODY,
    S16_CHAR_DONE,   
    S17_SINGLE_CHAR, 
    S18_ERROR        
};

Lexer::Lexer(istream& input) : in(input), line(1) {}

Token Lexer::getNextToken() {
        string lexeme;
        State state = S0_START;
        bool malformedChar = false;

        while (true) {
            switch (state) {

            case S0_START: {
                int ic = in.get();
                if (ic == EOF) return makeToken(TOK_EOF, "");
                char c = (char)ic;

                if (c == '\n') { line++; continue; }
                if (isspace((unsigned char)c)) continue;

                if (isalpha((unsigned char)c) || c == '_') {
                    lexeme += c; state = S1_IN_ID;
                }
                else if (isdigit((unsigned char)c)) {
                    lexeme += c; state = S2_IN_NUM;
                }
                else if (c == '<') { state = S3_LT_SEEN; }
                else if (c == '>') { state = S6_GT_SEEN; }
                else if (c == '=') { state = S9_ASSIGN_SEEN; }
                else if (c == '!') { state = S12_NOT_SEEN; }
                else if (c == '\'') {
                    lexeme += c;
                    state = S14_CHAR_OPEN;
                }
                else if (c == ';' || c == ',' || c == '(' || c == ')' ||
                         c == '{' || c == '}' || c == '+' || c == '-' ||
                         c == '*' || c == '/' || c == '%') {
                    lexeme += c; state = S17_SINGLE_CHAR;
                }
                else {
                    lexeme += c; state = S18_ERROR;
                }
                break;
            }

            case S1_IN_ID: {
                int ic = in.get();
                char c = (char)ic;
                if (ic != EOF && (isalnum((unsigned char)c) || c == '_')) {
                    lexeme += c; // stay in S1
                } else {
                    if (ic != EOF) in.unget(); // retract
                    return lookupIdOrKeyword(lexeme);
                }
                break;
            }

            case S2_IN_NUM: {
                int ic = in.get();
                char c = (char)ic;
                if (ic != EOF && isdigit((unsigned char)c)) {
                    lexeme += c; // stay in S2
                } else {
                    if (ic != EOF) in.unget(); // retract
                    return makeToken(TOK_NUM, lexeme);
                }
                break;
            }

            case S3_LT_SEEN: {
                int ic = in.get();
                if (ic == '=') { lexeme = "<="; state = S4_LE_DONE; }
                else { if (ic != EOF) in.unget(); state = S5_LT_DONE; }
                break;
            }
            case S4_LE_DONE: 
                return makeToken(TOK_LE, "<=");
            case S5_LT_DONE:
                return makeToken(TOK_LT, "<");

            case S6_GT_SEEN: {
                int ic = in.get();
                if (ic == '=') { lexeme = ">="; state = S7_GE_DONE; }
                else { if (ic != EOF) in.unget(); state = S8_GT_DONE; }
                break;
            }
            case S7_GE_DONE: 
                return makeToken(TOK_GE, ">=");
            case S8_GT_DONE: 
                return makeToken(TOK_GT, ">");

            case S9_ASSIGN_SEEN: {
                int ic = in.get();
                if (ic == '=') { lexeme = "=="; state = S10_EQ_DONE; }
                else { if (ic != EOF) in.unget(); state = S11_ASSIGN_DONE; }
                break;
            }
            case S10_EQ_DONE: 
                return makeToken(TOK_EQ, "==");
            case S11_ASSIGN_DONE: 
                return makeToken(TOK_ASSIGN, "=");

            case S12_NOT_SEEN: {
                int ic = in.get();
                if (ic == '=') { lexeme = "!="; state = S13_NE_DONE; }
                else {
                    if (ic != EOF) in.unget();
                    lexeme = "!"; state = S18_ERROR;
                }
                break;
            }
            case S13_NE_DONE: 
                return makeToken(TOK_NE, "!=");

            case S14_CHAR_OPEN: {
                int ic = in.get();
                if (ic != EOF && ic != '\n' && ic != '\r' && ic != '\'') {
                    lexeme += (char)ic;
                    state = S15_CHAR_BODY;
                } else {
                    if (ic != EOF) in.unget();
                    malformedChar = true;
                    state = S18_ERROR; 
                }
                break;
            }
            case S15_CHAR_BODY: {
                int ic = in.get();
                if (ic == '\'') {
                    lexeme += '\'';
                    state = S16_CHAR_DONE;
                }
                else {
                    if (ic != EOF) in.unget();
                    malformedChar = true;
                    state = S18_ERROR; 
                }
                break;
            }
            case S16_CHAR_DONE: 
                return makeToken(TOK_CHARCONST, lexeme);

            case S17_SINGLE_CHAR:   
                return makeToken(singleCharType(lexeme[0]), lexeme);

            case S18_ERROR:
                if (malformedChar) {
                    int ic;
                    while ((ic = in.get()) != EOF && ic != '\'' && ic != '\n') {
                    }
                    if (ic == '\n') in.unget();
                    malformedChar = false;
                }
                return makeToken(TOK_ERROR, lexeme);

            }
        }
}

Token Lexer::makeToken(TokenType t, const string& lex) {
    return Token{t, lex, line};
}

Token Lexer::lookupIdOrKeyword(const string& lex) {
    auto it = keywordTable.find(lex);
    if (it != keywordTable.end()) return makeToken(it->second, lex);
    return makeToken(TOK_ID, lex);
}

TokenType Lexer::singleCharType(char c) {
    switch (c) {
        case ';': return TOK_SEMI;
        case ',': return TOK_COMMA;
        case '(': return TOK_LPAREN;
        case ')': return TOK_RPAREN;
        case '{': return TOK_LBRACE;
        case '}': return TOK_RBRACE;
        case '+': return TOK_PLUS;
        case '-': return TOK_MINUS;
        case '*': return TOK_TIMES;
        case '/': return TOK_DIV;
        case '%': return TOK_MOD;
    }
    return TOK_ERROR;
}

#ifndef CMM_LEXER_NO_MAIN
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source-file.cmm>\n";
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cerr << "Error: cannot open file " << argv[1] << "\n";
        return 1;
    }

    Lexer lexer(file);
    cout << "TOKEN\t\tLEXEME\t\tLINE\n";
    cout << "\n\n\n";

    while (true) {
        Token tok = lexer.getNextToken();
        if (tok.type == TOK_EOF) break;

        cout << tokenName(tok.type);
        if (string(tokenName(tok.type)).length() < 8) cout << "\t";
        cout << "\t" << tok.lexeme << "\t\t" << tok.line << "\n";

        if (tok.type == TOK_ERROR) {
            cerr << "Lexical error at line " << tok.line
                 << ": unrecognized token '" << tok.lexeme << "'\n";
        }
    }

    return 0;
}
#endif
```

### 2.4 `src/parser.hpp`

```cpp
#ifndef CMM_PARSER_HPP
#define CMM_PARSER_HPP

#include "codegen.hpp"
#include "lexer.hpp"

#include <istream>
#include <stdexcept>
#include <string>

using namespace std;

class ParseError : public runtime_error {
public:
    explicit ParseError(const string& message);
};

class Parser {
public:
    explicit Parser(istream& input);

    void parse();

    CodeGen& codegen();
    const CodeGen& codegen() const;

private:
    Lexer lexer;
    Token current;
    CodeGen gen;

    void advance();
    void rejectLexicalError() const;
    [[noreturn]] void syntaxError(const string& reason) const;
    bool accept(TokenType type);
    void expect(TokenType type, const string& description);

    string parseValueType();
    void parseExternalDeclaration();
    void parseFunctionTail();
    void parseVariableTail(const string& type);
    void parseCompoundStatement();
    void parseStatement();

    string parseExpression();
    static bool isRelationalOperator(TokenType type);
    string parseAdditiveExpression();
    string parseTerm();
    string parseFactor();
};

#endif
```

### 2.5 `src/parser.cpp`

```cpp
#include "parser.hpp"

#include <sstream>

using namespace std;

ParseError::ParseError(const string& message) : runtime_error(message) {}

Parser::Parser(istream& input) : lexer(input), current(lexer.getNextToken()) {
    rejectLexicalError();
}

void Parser::parse() {
    while (current.type != TOK_EOF) {
        parseExternalDeclaration();
    }
}

CodeGen& Parser::codegen() { return gen; }
const CodeGen& Parser::codegen() const { return gen; }

void Parser::advance() {
    current = lexer.getNextToken();
    rejectLexicalError();
}

void Parser::rejectLexicalError() const {
    if (current.type == TOK_ERROR) {
        ostringstream message;
        message << "Lexical error at line " << current.line
                << ": unrecognized token '" << current.lexeme << "'";
        throw ParseError(message.str());
    }
}

[[noreturn]] void Parser::syntaxError(const string& reason) const {
    ostringstream message;
    message << "Syntax error at line " << current.line << ": " << reason
            << " near ";
    if (current.type == TOK_EOF) {
        message << "end of file";
    } else {
        message << "'" << current.lexeme << "'";
    }
    throw ParseError(message.str());
}

bool Parser::accept(TokenType type) {
    if (current.type != type) return false;
    advance();
    return true;
}

void Parser::expect(TokenType type, const string& description) {
    if (current.type != type) {
        syntaxError("expected " + description);
    }
    advance();
}

// ---- Declarations ---------------------------------------------------------

string Parser::parseValueType() {
    if (current.type != TOK_KW_INT && current.type != TOK_KW_CHAR) {
        syntaxError("expected 'int' or 'char'");
    }
    const string type = current.lexeme;
    advance();
    return type;
}

void Parser::parseExternalDeclaration() {
    const bool isVoid = current.type == TOK_KW_VOID;
    string type;
    if (isVoid) {
        type = "void";
        advance();
    } else {
        type = parseValueType();
    }

    const string name = current.lexeme;
    expect(TOK_ID, "identifier");

    if (current.type == TOK_LPAREN) {
        gen.addSymbol(name, "function", type);
        gen.emit("func " + name);
        parseFunctionTail();
        gen.emit("endfunc " + name);
        return;
    }

    if (isVoid) {
        syntaxError("'void' is only valid as a function return type");
    }
    gen.addSymbol(name, "variable", type);
    parseVariableTail(type);
}

void Parser::parseFunctionTail() {
    expect(TOK_LPAREN, "'('");
    if (current.type == TOK_KW_VOID) {
        advance();
    } else if (current.type != TOK_RPAREN) {
        string type = parseValueType();
        const string first = current.lexeme;
        expect(TOK_ID, "parameter name");
        gen.addSymbol(first, "parameter", type);
        while (accept(TOK_COMMA)) {
            type = parseValueType();
            const string param = current.lexeme;
            expect(TOK_ID, "parameter name");
            gen.addSymbol(param, "parameter", type);
        }
    }
    expect(TOK_RPAREN, "')'");
    parseCompoundStatement();
}

void Parser::parseVariableTail(const string& type) {
    while (accept(TOK_COMMA)) {
        const string name = current.lexeme;
        expect(TOK_ID, "identifier");
        gen.addSymbol(name, "variable", type);
    }
    expect(TOK_SEMI, "';'");
}

void Parser::parseCompoundStatement() {
    expect(TOK_LBRACE, "'{'");
    while (current.type == TOK_KW_INT || current.type == TOK_KW_CHAR) {
        const string type = parseValueType();
        const string name = current.lexeme;
        expect(TOK_ID, "identifier");
        gen.addSymbol(name, "variable", type);
        parseVariableTail(type);
    }
    while (current.type != TOK_RBRACE && current.type != TOK_EOF) {
        parseStatement();
    }
    expect(TOK_RBRACE, "'}'");
}

// ---- Statements -----------------------------------------------------------

void Parser::parseStatement() {
    switch (current.type) {
        case TOK_ID: {
            const string target = current.lexeme;
            advance();
            expect(TOK_ASSIGN, "'='");
            const string value = parseExpression();
            expect(TOK_SEMI, "';'");
            gen.emit(target + " = " + value);
            return;
        }
        case TOK_KW_IF: {
            advance();
            expect(TOK_LPAREN, "'('");
            const string condition = parseExpression();
            expect(TOK_RPAREN, "')'");

            const string elseLabel = gen.newLabel();
            gen.emit("ifFalse " + condition + " goto " + elseLabel);
            parseCompoundStatement();
            if (accept(TOK_KW_ELSE)) {
                const string endLabel = gen.newLabel();
                gen.emit("goto " + endLabel);
                gen.emit(elseLabel + ":");
                parseCompoundStatement();
                gen.emit(endLabel + ":");
            } else {
                gen.emit(elseLabel + ":");
            }
            return;
        }
        case TOK_KW_WHILE: {
            advance();
            const string startLabel = gen.newLabel();
            gen.emit(startLabel + ":");
            expect(TOK_LPAREN, "'('");
            const string condition = parseExpression();
            expect(TOK_RPAREN, "')'");
            const string endLabel = gen.newLabel();
            gen.emit("ifFalse " + condition + " goto " + endLabel);
            parseCompoundStatement();
            gen.emit("goto " + startLabel);
            gen.emit(endLabel + ":");
            return;
        }
        case TOK_KW_IN: {
            advance();
            expect(TOK_LPAREN, "'('");
            const string value = current.lexeme;
            expect(TOK_ID, "identifier");
            expect(TOK_RPAREN, "')'");
            expect(TOK_SEMI, "';'");
            gen.emit("in " + value);
            return;
        }
        case TOK_KW_OUT: {
            advance();
            expect(TOK_LPAREN, "'('");
            const string value = parseExpression();
            expect(TOK_RPAREN, "')'");
            expect(TOK_SEMI, "';'");
            gen.emit("out " + value);
            return;
        }
        case TOK_KW_RETURN:
            advance();
            if (current.type != TOK_SEMI) {
                const string value = parseExpression();
                gen.emit("return " + value);
            } else {
                gen.emit("return");
            }
            expect(TOK_SEMI, "';'");
            return;
        case TOK_LBRACE:
            parseCompoundStatement();
            return;
        case TOK_SEMI:
            advance();
            return;
        default:
            syntaxError("expected a statement");
    }
}

// ---- Expressions ----------------------------------------------------------
// Each expression returns the name of the place holding its value:
// an identifier, a constant, or a generated temporary.

string Parser::parseExpression() {
    string left = parseAdditiveExpression();
    if (isRelationalOperator(current.type)) {
        const string op = current.lexeme;
        advance();
        const string right = parseAdditiveExpression();
        const string temp = gen.newTemp();
        gen.emit(temp + " = " + left + " " + op + " " + right);
        return temp;
    }
    return left;
}

bool Parser::isRelationalOperator(TokenType type) {
    return type == TOK_LT || type == TOK_GT || type == TOK_LE ||
           type == TOK_GE || type == TOK_EQ || type == TOK_NE;
}

string Parser::parseAdditiveExpression() {
    string left = parseTerm();
    while (current.type == TOK_PLUS || current.type == TOK_MINUS) {
        const string op = current.lexeme;
        advance();
        const string right = parseTerm();
        const string temp = gen.newTemp();
        gen.emit(temp + " = " + left + " " + op + " " + right);
        left = temp;
    }
    return left;
}

string Parser::parseTerm() {
    string left = parseFactor();
    while (current.type == TOK_TIMES || current.type == TOK_DIV ||
           current.type == TOK_MOD) {
        const string op = current.lexeme;
        advance();
        const string right = parseFactor();
        const string temp = gen.newTemp();
        gen.emit(temp + " = " + left + " " + op + " " + right);
        left = temp;
    }
    return left;
}

string Parser::parseFactor() {
    switch (current.type) {
        case TOK_ID:
        case TOK_NUM:
        case TOK_CHARCONST: {
            const string place = current.lexeme;
            advance();
            return place;
        }
        case TOK_LPAREN: {
            advance();
            const string place = parseExpression();
            expect(TOK_RPAREN, "')'");
            return place;
        }
        default:
            syntaxError("expected an identifier, constant, or parenthesized expression");
    }
}
```

### 2.6 `src/codegen.hpp`

```cpp
#ifndef CMM_CODEGEN_HPP
#define CMM_CODEGEN_HPP

#include <string>
#include <vector>

using namespace std;

// Collects three-address code and symbol-table entries while the parser walks
// the source. It performs no syntax checking of its own.
class CodeGen {
public:
    string newTemp();
    string newLabel();

    void emit(const string& line);
    void addSymbol(const string& name, const string& kind, const string& type);

    void writeOutputs(const string& tacPath, const string& symPath) const;

private:
    struct Symbol {
        string name;
        string kind;   // function, parameter, variable
        string type;   // int, char, void
        int address;
        string initial;
    };

    vector<string> tac;
    vector<Symbol> symbols;
    int nextAddress = 1000;
    int tempCount = 0;
    int labelCount = 0;
};

#endif
```

### 2.7 `src/codegen.cpp`

```cpp
#include "codegen.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>

using namespace std;

string CodeGen::newTemp() {
    return "t" + to_string(++tempCount);
}

string CodeGen::newLabel() {
    return "L" + to_string(++labelCount);
}

void CodeGen::emit(const string& line) {
    tac.push_back(line);
}

void CodeGen::addSymbol(const string& name, const string& kind, const string& type) {
    symbols.push_back(Symbol{name, kind, type, nextAddress, "-"});
    nextAddress += 4;
}

void CodeGen::writeOutputs(const string& tacPath, const string& symPath) const {
    ofstream tacOut(tacPath);
    if (!tacOut) throw runtime_error("cannot open " + tacPath);
    for (const string& line : tac) {
        tacOut << line << '\n';
    }

    ofstream symOut(symPath);
    if (!symOut) throw runtime_error("cannot open " + symPath);
    symOut << left
           << setw(16) << "Name"
           << setw(12) << "Kind"
           << setw(8)  << "Type"
           << setw(10) << "Address"
           << "Initial\n";
    symOut << string(58, '-') << '\n';
    for (const Symbol& symbol : symbols) {
        symOut << left
               << setw(16) << symbol.name
               << setw(12) << symbol.kind
               << setw(8)  << symbol.type
               << setw(10) << symbol.address
               << symbol.initial << '\n';
    }
}
```

## 3 · Generated three-address code (`tac.txt`)

```text
func add
t1 = a + b
sum = t1
return sum
endfunc add
func main
in x
in y
t2 = x >= y
ifFalse t2 goto L1
t3 = x - y
result = t3
goto L2
L1:
t4 = y - x
result = t4
L2:
L3:
t5 = x != 0
ifFalse t5 goto L4
t6 = x - 1
x = t6
goto L3
L4:
grade = 'A'
out result
out grade
return 0
endfunc main
```

## 4 · Generated symbol table (`symtab.txt`)

```text
Name            Kind        Type    Address   Initial
----------------------------------------------------------
add             function    int     1000      -
a               parameter   int     1004      -
b               parameter   int     1008      -
sum             variable    int     1012      -
main            function    int     1016      -
x               variable    int     1020      -
y               variable    int     1024      -
result          variable    int     1028      -
grade           variable    char    1032      -
```

---

End of inputs. Everything above is verbatim from the repository as of generation time.
