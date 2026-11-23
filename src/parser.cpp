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

// ---- Statements -----------------------------------------------------------

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

// ---- Expressions ----------------------------------------------------------
// Each expression returns the name of the place holding its value:
// an identifier, a constant, or a generated temporary.

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
