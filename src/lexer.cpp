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
