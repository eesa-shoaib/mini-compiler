# Lexer Implementation

*Assignment phase 4.*

## 1. Purpose

`src/lexer.cpp` reads a C-- source file character by character and groups
characters into tokens. Each token contains:

```cpp
struct Token {
    TokenType type;
    string lexeme;
    int line;
};
```

- `type` identifies the token category.
- `lexeme` stores the exact source text recognized.
- `line` stores the source line where the token begins.

The lexer recognizes:

- Keywords: `int`, `char`, `if`, `else`, `while`, `in`, `out`, `return`, `void`
- Identifiers
- Integer constants
- Character constants
- Assignment and relational operators
- Arithmetic operators
- Delimiters
- Errors

The lexer does not validate complete declarations, expressions, functions, or
control statements. That work belongs to a parser.

## 2. Main Components

### Token type enumeration

`TokenType` defines every token returned by the lexer:

```cpp
TOK_KW_INT
TOK_ID
TOK_NUM
TOK_CHARCONST
TOK_ASSIGN
TOK_LT
TOK_LE
TOK_EQ
TOK_SEMI
TOK_EOF
TOK_ERROR
```

`tokenName()` converts each enum value into readable output text.

### Keyword table

Identifiers are scanned first. After scanning, `lookupIdOrKeyword()` checks the
lexeme in `keywordTable`.

For example:

```text
int       -> KW_INT
while     -> KW_WHILE
counter   -> ID
```

This allows keywords and identifiers to use one shared scanning path.

### Lexer class

`Lexer` stores:

```cpp
istream& in;
int line;
```

`getNextToken()` performs the state-machine scan. `makeToken()` creates tokens,
and `singleCharType()` maps one-character symbols to token types.

## 3. General Token Flow

```text
getNextToken()
    |
    v
S0_START
    |
    +-- whitespace        -> skip and remain in S0
    +-- letter or _       -> S1_IN_ID
    +-- digit             -> S2_IN_NUM
    +-- <                 -> S3_LT_SEEN
    +-- >                 -> S6_GT_SEEN
    +-- =                 -> S9_ASSIGN_SEEN
    +-- !                 -> S12_NOT_SEEN
    +-- '                 -> S14_CHAR_OPEN
    +-- single symbol     -> S17_SINGLE_CHAR
    +-- unsupported char  -> S18_ERROR
```

When a state reads one character too far, it calls `in.unget()` to retract that
character. The next call to `getNextToken()` then processes it normally.

## 4. State-by-State Flow

### S0_START: Start and dispatch

`S0_START` reads the next input character.

1. At end of file, return `TOK_EOF`.
2. Skip spaces, tabs, and newlines.
3. Increment `line` when a newline is skipped.
4. Select the state matching the first character of the token.

This state does not directly return ordinary tokens except `TOK_EOF`.

### S1_IN_ID: Identifier loop

Entered when the first character is a letter or underscore.

```text
letter or _ -> S1
letter, digit, or _ -> remain in S1
other -> retract, return ID or keyword
```

Example:

```text
input:  count;
tokens: ID("count"), SEMI(";")
```

The semicolon is read while checking whether the identifier is complete, so it
is retracted before the identifier token is returned.

### S2_IN_NUM: Integer loop

Entered when the first character is a digit.

```text
digit -> remain in S2
non-digit -> retract, return NUM
```

Example:

```text
input:  123;
tokens: NUM("123"), SEMI(";")
```

Only integer constants are supported. Decimal points are not recognized as
part of a number.

### S3_LT_SEEN: Less-than lookahead

Entered after reading `<`.

```text
next character = = -> S4_LE_DONE
otherwise       -> retract, S5_LT_DONE
```

### S4_LE_DONE: Less-than-or-equal accepting state

Returns:

```text
LE("<=")
```

### S5_LT_DONE: Less-than accepting state

Returns:

```text
LT("<")
```

The second character was retracted before entering this state.

### S6_GT_SEEN: Greater-than lookahead

Entered after reading `>`.

```text
next character = = -> S7_GE_DONE
otherwise       -> retract, S8_GT_DONE
```

### S7_GE_DONE: Greater-than-or-equal accepting state

Returns:

```text
GE(">=")
```

### S8_GT_DONE: Greater-than accepting state

Returns:

```text
GT(">")
```

### S9_ASSIGN_SEEN: Assignment/equality lookahead

Entered after reading `=`.

```text
next character = = -> S10_EQ_DONE
otherwise       -> retract, S11_ASSIGN_DONE
```

### S10_EQ_DONE: Equality accepting state

Returns:

```text
EQ("==")
```

### S11_ASSIGN_DONE: Assignment accepting state

Returns:

```text
ASSIGN("=")
```

### S12_NOT_SEEN: Not-equal lookahead

Entered after reading `!`.

```text
next character = = -> S13_NE_DONE
otherwise       -> S18_ERROR
```

Standalone `!` is invalid because logical operators are outside the C-- subset.

### S13_NE_DONE: Not-equal accepting state

Returns:

```text
NE("!=")
```

### S14_CHAR_OPEN: Character literal opening

Entered after reading the opening single quote.

The lexer accepts one character if it is:

- not another single quote
- not a newline
- not end of file

Valid input moves to `S15_CHAR_BODY`.

Invalid input moves to `S18_ERROR`.

### S15_CHAR_BODY: Character literal closing

The lexer reads one more character.

```text
closing ' -> S16_CHAR_DONE
anything else -> S18_ERROR
```

Examples:

```text
'a'  -> CHARCONST("'a'")
'7'  -> CHARCONST("'7'")
' '  -> CHARCONST("' '")
```

The complete lexeme, including both quotes, is preserved.

### S16_CHAR_DONE: Character constant accepting state

Returns:

```text
CHARCONST("'a'")
```

Only one character is allowed inside the quotes.

### S17_SINGLE_CHAR: Single-character accepting state

The following characters enter `S17`:

```text
; , ( ) { } + - * / %
```

`singleCharType()` maps the character to its token type:

```text
; -> SEMI
, -> COMMA
( -> LPAREN
) -> RPAREN
{ -> LBRACE
} -> RBRACE
+ -> PLUS
- -> MINUS
* -> TIMES
/ -> DIV
% -> MOD
```

### S18_ERROR: Error accepting state

Unsupported characters produce an `ERROR` token.

Examples:

```text
@ -> ERROR("@")
! -> ERROR("!")
```

For malformed character literals, the lexer consumes input until the closing
quote or line end. This prevents the remaining characters of one malformed
literal from producing multiple misleading errors.

## 5. Retract Operation

The lexer uses one-character lookahead. When a character belongs to the next
token, it is returned to the input stream using:

```cpp
in.unget();
```

Example for `<` followed by `x`:

```text
read <     -> possible <=
read x     -> not =
retract x  -> x remains for next token
return LT
```

The next call starts at `x`.

## 6. Accepting States

Accepting states return a complete token:

```text
S4, S5       relational <
S7, S8       relational >
S10, S11     equality or assignment
S13          not-equal
S16          character constant
S17          single-character token
S18          error
```

Identifier and number states return tokens after their terminating
lookahead character is retracted:

```text
S1 -> ID or keyword
S2 -> NUM
```

## 7. Driver Flow

`main()` performs these steps:

1. Check that a source-file argument was provided.
2. Open the source file.
3. Construct a `Lexer`.
4. Repeatedly call `getNextToken()`.
5. Print token type, lexeme, and line number.
6. Stop when `TOK_EOF` is returned.
7. Print an error message for every `TOK_ERROR`.

```text
source file
    |
    v
Lexer
    |
    v
getNextToken() repeatedly
    |
    v
token table output
    |
    v
stop at EOF
```

## 8. Example

Input:

```cpp
int count = 10;
if (count >= 5) {
    out(count);
}
```

Important state flow:

```text
int       S0 -> S1 -> keyword lookup -> KW_INT
count     S0 -> S1 -> ID
=         S0 -> S9 -> S11 -> ASSIGN
10        S0 -> S2 -> NUM
if        S0 -> S1 -> keyword lookup -> KW_IF
>=        S0 -> S6 -> S7 -> GE
{         S0 -> S17 -> LBRACE
out       S0 -> S1 -> keyword lookup -> KW_OUT
(         S0 -> S17 -> LPAREN
count     S0 -> S1 -> ID
)         S0 -> S17 -> RPAREN
;         S0 -> S17 -> SEMI
}         S0 -> S17 -> RBRACE
```

The lexer recognizes each token independently. It does not decide whether the
whole sequence forms a valid C-- program.
