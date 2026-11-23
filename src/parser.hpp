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

#endif
