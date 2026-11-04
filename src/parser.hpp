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
