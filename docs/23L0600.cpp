#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <map>
#include <set>
#include <cctype>
#include <istream>
#include <string>
#include <vector>

using namespace std;

class CodeGen {
public:
    string newTemp();
    string newLabel();

    void emit(const string& line);
    void addSymbol(const string& name, const string& kind, const string& type,
                   bool isGlobal = false);

    const vector<string>& tacLines() const;
    map<string, string> symbolTypes() const;
    set<string> globalNames() const;

    void writeOutputs(const string& tacPath, const string& symPath) const;

private:
    struct Symbol {
        string name;
        string kind;
        string type;
        int address;
        string initial;
    };

    vector<string> tac;
    vector<Symbol> symbols;
    set<string> globals;
    int nextAddress = 1000;
    int tempCount = 0;
    int labelCount = 0;
};

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

class ParseError : public runtime_error {
public:
    explicit ParseError(const string& message);
};

class Parser {
public:
    explicit Parser(istream& input);

    void parse();

    CodeGen& codegen();

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
    void parseParameter();
    void parseVariableTail(const string& type, bool isGlobal = false);
    void parseVariableDeclaration(const string& type, bool isGlobal = false);
    void parseCompoundStatement();
    void parseStatement();
    int parseArguments();

    string parseExpression();
    string emitBinary(const string& left, const string& op, const string& right);
    static bool isRelationalOperator(TokenType type);
    string parseAdditiveExpression();
    string parseTerm();
    string parseFactor();
};

class VMError : public runtime_error {
public:
    explicit VMError(const string& message);
};

class VirtualMachine {
public:
    VirtualMachine(const vector<string>& tac,
                   const map<string, string>& types,
                   const set<string>& globalNames);

    void run();

private:
    struct Function {
        size_t begin = 0;
        size_t end = 0;
    };

    vector<string> code;
    map<string, size_t> labels;
    map<string, Function> functions;
    map<string, vector<string>> formals;
    map<string, string> types;
    map<string, int> globals;

    void load();
    int execute(const string& name, map<string, int>& locals);
    int call(const string& nameToken, const string& countToken,
             size_t pc, vector<int>& pending);
    void printOutput(const string& token, map<string, int>& locals) const;
    int operand(const string& token, map<string, int>& locals) const;
    void assign(const string& name, int value, map<string, int>& locals);
    int apply(const string& op, int left, int right, size_t pc) const;
    size_t label(const string& name) const;
    VMError fail(size_t pc, const string& message) const;

    static bool isCharLiteral(const string& token);
    static vector<string> split(const string& line);
};

string CodeGen::newTemp() {
    return "t" + to_string(++tempCount);
}

string CodeGen::newLabel() {
    return "L" + to_string(++labelCount);
}

void CodeGen::emit(const string& line) {
    tac.push_back(line);
}

void CodeGen::addSymbol(const string& name, const string& kind, const string& type,
                        bool isGlobal) {
    symbols.push_back(Symbol{name, kind, type, nextAddress, "-"});
    if (isGlobal && kind == "variable") globals.insert(name);
    nextAddress += 4;
}

const vector<string>& CodeGen::tacLines() const {
    return tac;
}

map<string, string> CodeGen::symbolTypes() const {
    map<string, string> types;
    for (const Symbol& symbol : symbols) {
        types[symbol.name] = symbol.type;
    }
    return types;
}

set<string> CodeGen::globalNames() const {
    return globals;
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
                    lexeme += c;
                } else {
                    if (ic != EOF) in.unget();
                    return lookupIdOrKeyword(lexeme);
                }
                break;
            }

            case S2_IN_NUM: {
                int ic = in.get();
                char c = (char)ic;
                if (ic != EOF && isdigit((unsigned char)c)) {
                    lexeme += c;
                } else {
                    if (ic != EOF) in.unget();
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
    const string type = isVoid ? "void" : parseValueType();
    if (isVoid) advance();

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
    gen.addSymbol(name, "variable", type, true);
    parseVariableTail(type, true);
}

void Parser::parseFunctionTail() {
    expect(TOK_LPAREN, "'('");
    if (current.type == TOK_KW_VOID) {
        advance();
    } else if (current.type != TOK_RPAREN) {
        do {
            parseParameter();
        } while (accept(TOK_COMMA));
    }
    expect(TOK_RPAREN, "')'");
    parseCompoundStatement();
}

void Parser::parseParameter() {
    const string type = parseValueType();
    const string name = current.lexeme;
    expect(TOK_ID, "parameter name");
    gen.addSymbol(name, "parameter", type);
    gen.emit("param " + name);
}

void Parser::parseVariableTail(const string& type, bool isGlobal) {
    while (accept(TOK_COMMA)) {
        parseVariableDeclaration(type, isGlobal);
    }
    expect(TOK_SEMI, "';'");
}

void Parser::parseVariableDeclaration(const string& type, bool isGlobal) {
    const string name = current.lexeme;
    expect(TOK_ID, "identifier");
    gen.addSymbol(name, "variable", type, isGlobal);
}

void Parser::parseCompoundStatement() {
    expect(TOK_LBRACE, "'{'");
    while (current.type == TOK_KW_INT || current.type == TOK_KW_CHAR) {
        const string type = parseValueType();
        parseVariableDeclaration(type, false);
        parseVariableTail(type, false);
    }
    while (current.type != TOK_RBRACE && current.type != TOK_EOF) {
        parseStatement();
    }
    expect(TOK_RBRACE, "'}'");
}

void Parser::parseStatement() {
    switch (current.type) {
        case TOK_ID: {
            const string name = current.lexeme;
            advance();
            if (current.type == TOK_LPAREN) {
                const int argc = parseArguments();
                gen.emit("call " + name + ", " + to_string(argc));
                expect(TOK_SEMI, "';'");
                return;
            }
            expect(TOK_ASSIGN, "'='");
            const string value = parseExpression();
            expect(TOK_SEMI, "';'");
            gen.emit(name + " = " + value);
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
            if (current.type == TOK_SEMI) {
                gen.emit("return");
            } else {
                gen.emit("return " + parseExpression());
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

string Parser::parseExpression() {
    const string left = parseAdditiveExpression();
    if (!isRelationalOperator(current.type)) return left;
    const string op = current.lexeme;
    advance();
    return emitBinary(left, op, parseAdditiveExpression());
}

string Parser::emitBinary(const string& left, const string& op, const string& right) {
    const string temp = gen.newTemp();
    gen.emit(temp + " = " + left + " " + op + " " + right);
    return temp;
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
        left = emitBinary(left, op, parseTerm());
    }
    return left;
}

string Parser::parseTerm() {
    string left = parseFactor();
    while (current.type == TOK_TIMES || current.type == TOK_DIV ||
           current.type == TOK_MOD) {
        const string op = current.lexeme;
        advance();
        left = emitBinary(left, op, parseFactor());
    }
    return left;
}

string Parser::parseFactor() {
    switch (current.type) {
        case TOK_ID: {
            const string name = current.lexeme;
            advance();
            if (current.type == TOK_LPAREN) {
                const int argc = parseArguments();
                const string temp = gen.newTemp();
                gen.emit(temp + " = call " + name + ", " + to_string(argc));
                return temp;
            }
            return name;
        }
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

int Parser::parseArguments() {
    expect(TOK_LPAREN, "'('");
    int count = 0;
    if (current.type != TOK_RPAREN) {
        do {
            gen.emit("push " + parseExpression());
            ++count;
        } while (accept(TOK_COMMA));
    }
    expect(TOK_RPAREN, "')'");
    return count;
}

VMError::VMError(const string& message) : runtime_error(message) {}

VirtualMachine::VirtualMachine(const vector<string>& tac,
                               const map<string, string>& types,
                               const set<string>& globalNames)
    : code(tac), types(types) {
    for (const string& name : globalNames) {
        globals[name] = 0;
    }
    load();
}

vector<string> VirtualMachine::split(const string& line) {
    vector<string> tokens;
    string token;
    bool quoted = false;
    for (char c : line) {
        if (c == '\'') {
            quoted = !quoted;
            token += c;
        } else if (isspace((unsigned char)c) && !quoted) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

void VirtualMachine::load() {
    string openFunction;
    for (size_t i = 0; i < code.size(); ++i) {
        const vector<string> tokens = split(code[i]);
        if (tokens.empty()) continue;
        if (tokens.size() == 2 && tokens[0] == "func") {
            if (!openFunction.empty()) {
                throw VMError("nested function definition '" + tokens[1] + "'");
            }
            openFunction = tokens[1];
            functions[openFunction].begin = i + 1;
        } else if (tokens.size() == 2 && tokens[0] == "endfunc") {
            if (openFunction != tokens[1]) {
                throw VMError("endfunc '" + tokens[1] + "' does not match func '" +
                              openFunction + "'");
            }
            functions[tokens[1]].end = i;
            openFunction.clear();
        } else if (tokens.size() == 2 && tokens[0] == "param") {
            if (openFunction.empty()) throw VMError("param outside a function");
            formals[openFunction].push_back(tokens[1]);
        } else if (tokens.size() == 1 && tokens[0].back() == ':') {
            labels[tokens[0].substr(0, tokens[0].size() - 1)] = i;
        }
    }
    if (!openFunction.empty()) {
        throw VMError("missing endfunc for '" + openFunction + "'");
    }
}

void VirtualMachine::run() {
    if (functions.find("main") == functions.end()) {
        throw VMError("no 'main' function in the TAC");
    }
    map<string, int> locals;
    execute("main", locals);
}

int VirtualMachine::execute(const string& name, map<string, int>& locals) {
    const Function& function = functions.at(name);
    size_t pc = function.begin;
    vector<int> pending;

    while (pc < function.end) {
        const size_t line = pc++;
        const vector<string> tokens = split(code[line]);
        if (tokens.empty() || tokens[0].back() == ':') continue;
        const string& op = tokens[0];

        if (op == "goto") {
            pc = label(tokens[1]);
        } else if (op == "ifFalse") {
            if (operand(tokens[1], locals) == 0) pc = label(tokens[3]);
        } else if (op == "in") {
            int input;
            if (!(cin >> input)) throw fail(line, "expected an integer input");
            assign(tokens[1], input, locals);
        } else if (op == "out") {
            printOutput(tokens[1], locals);
        } else if (op == "push") {
            pending.push_back(operand(tokens[1], locals));
        } else if (op == "call") {
            call(tokens[1], tokens[2], line, pending);
        } else if (op == "return") {
            return tokens.size() > 1 ? operand(tokens[1], locals) : 0;
        } else if (op == "param") {
            continue;
        } else if (tokens.size() >= 3 && tokens[1] == "=") {
            if (tokens.size() == 5 && tokens[2] == "call") {
                assign(tokens[0], call(tokens[3], tokens[4], line, pending), locals);
            } else if (tokens.size() == 3) {
                assign(tokens[0], operand(tokens[2], locals), locals);
            } else if (tokens.size() == 5) {
                assign(tokens[0],
                       apply(tokens[3],
                             operand(tokens[2], locals),
                             operand(tokens[4], locals),
                             line),
                       locals);
            } else {
                throw fail(line, "unknown TAC instruction");
            }
        } else {
            throw fail(line, "unknown TAC instruction");
        }
    }
    return 0;
}

int VirtualMachine::call(const string& nameToken, const string& countToken,
                         size_t pc, vector<int>& pending) {
    string name = nameToken;
    if (!name.empty() && name.back() == ',') name.pop_back();

    int count;
    try {
        count = stoi(countToken);
    } catch (const exception&) {
        throw fail(pc, "bad argument count '" + countToken + "'");
    }
    if (count < 0 || static_cast<size_t>(count) > pending.size()) {
        throw VMError("not enough arguments pushed for call");
    }

    map<string, Function>::const_iterator entry = functions.find(name);
    if (entry == functions.end()) {
        throw VMError("call to undefined function '" + name + "'");
    }
    const vector<string>& parameters = formals[name];
    if (parameters.size() != static_cast<size_t>(count)) {
        throw VMError("function '" + name + "' expects " + to_string(parameters.size()) +
                      " argument(s) but got " + to_string(count));
    }

    map<string, int> locals;
    for (int i = 0; i < count; ++i) {
        locals[parameters[i]] = pending[pending.size() - count + i];
    }
    pending.resize(pending.size() - count);
    return execute(name, locals);
}

void VirtualMachine::printOutput(const string& token, map<string, int>& locals) const {
    if (isCharLiteral(token)) {
        cout << token[1] << '\n';
        return;
    }
    const int result = operand(token, locals);
    if (types.count(token) && types.at(token) == "char") {
        cout << static_cast<char>(result) << '\n';
    } else {
        cout << result << '\n';
    }
}

int VirtualMachine::operand(const string& token, map<string, int>& locals) const {
    if (isCharLiteral(token)) {
        return static_cast<unsigned char>(token[1]);
    }

    size_t start = (!token.empty() && token[0] == '-') ? 1 : 0;
    bool numeric = start < token.size();
    for (size_t i = start; numeric && i < token.size(); ++i) {
        if (!isdigit(static_cast<unsigned char>(token[i]))) numeric = false;
    }
    if (numeric) return stoi(token);

    map<string, int>::const_iterator local = locals.find(token);
    if (local != locals.end()) return local->second;

    map<string, int>::const_iterator global = globals.find(token);
    if (global != globals.end()) return global->second;

    return 0;
}

void VirtualMachine::assign(const string& name, int value, map<string, int>& locals) {
    if (locals.find(name) != locals.end()) {
        locals[name] = value;
    } else if (globals.find(name) != globals.end()) {
        globals[name] = value;
    } else {
        locals[name] = value;
    }
}

int VirtualMachine::apply(const string& op, int left, int right, size_t pc) const {
    if (op == "+") return left + right;
    if (op == "-") return left - right;
    if (op == "*") return left * right;
    if (op == "/") {
        if (right == 0) throw fail(pc, "division by zero");
        return left / right;
    }
    if (op == "%") {
        if (right == 0) throw fail(pc, "modulo by zero");
        return left % right;
    }
    if (op == "<") return left < right;
    if (op == ">") return left > right;
    if (op == "<=") return left <= right;
    if (op == ">=") return left >= right;
    if (op == "==") return left == right;
    if (op == "!=") return left != right;
    throw fail(pc, "unknown operator '" + op + "'");
}

size_t VirtualMachine::label(const string& name) const {
    map<string, size_t>::const_iterator entry = labels.find(name);
    if (entry == labels.end()) {
        throw VMError("unknown label '" + name + "'");
    }
    return entry->second;
}

VMError VirtualMachine::fail(size_t pc, const string& message) const {
    return VMError(message + " near: " + code[pc]);
}

bool VirtualMachine::isCharLiteral(const string& token) {
    return token.size() >= 2 && token.front() == '\'' && token.back() == '\'';
}

int main(int argc, char* argv[]) {
    bool run = false;
    string source;

    for (int i = 1; i < argc; ++i) {
        const string argument = argv[i];
        if (argument == "--run") {
            run = true;
        } else if (!argument.empty() && argument[0] == '-') {
            cerr << "Unknown option: " << argument << "\n";
            return 1;
        } else if (source.empty()) {
            source = argument;
        } else {
            cerr << "Only one source file may be given\n";
            return 1;
        }
    }

    if (source.empty()) {
        cerr << "Usage: " << argv[0] << " [--run] <source-file.cmm>\n";
        return 1;
    }

    ifstream file(source);
    if (!file) {
        cerr << "Error: cannot open file " << source << "\n";
        return 1;
    }

    try {
        Parser parser(file);
        parser.parse();

        CodeGen& gen = parser.codegen();
        gen.writeOutputs("tac.txt", "symtab.txt");
        cout << "Parsing successful.\n"
             << "TAC written to tac.txt\n"
             << "Symbol table written to symtab.txt\n";

        if (run) {
            VirtualMachine vm(gen.tacLines(), gen.symbolTypes(), gen.globalNames());
            vm.run();
        }
    } catch (const ParseError& error) {
        cerr << error.what() << '\n';
        return 1;
    } catch (const VMError& error) {
        cerr << "Runtime error: " << error.what() << '\n';
        return 1;
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
