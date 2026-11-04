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
