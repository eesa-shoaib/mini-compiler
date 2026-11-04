# Grammar and Recursive-Descent Parser

*Assignment phase 5.*

This grammar extends the token set and sample constructs in
[`01-language-spec.md`](01-language-spec.md). It describes the subset accepted
by `src/parser.cpp`.

## 1. Context-Free Grammar (CFG)

Earlier names `LPAREN`, `RPAREN`, `LBRACE`, and `RBRACE` meant `(`, `)`, `{`,
and `}`. Grammar now uses source symbols directly. `T` means `int` or `char`;
`R` means any relational operator; `ε` means empty. Quoted items are literal
terminals.

```text
P  -> D P | ε
D  -> T ID X | "void" ID F
X  -> F | V
F  -> "(" A ")" B
A  -> L | "void" | ε
L  -> T ID Q
Q  -> "," T ID Q | ε
V  -> C ";"
C  -> "," ID C | ε
T  -> "int" | "char"
B  -> "{" G H "}"
G  -> T ID V G | ε
H  -> S H | ε
S  -> ID "=" E ";"
    | "if" "(" E ")" B O
    | "while" "(" E ")" B
    | "in" "(" ID ")" ";"
    | "out" "(" E ")" ";"
    | "return" U
    | B
    | ";"
O  -> "else" B | ε
U  -> E ";" | ";"
E  -> N | N R N
N  -> N "+" M | N "-" M | M
M  -> M "*" Z | M "/" Z | M "%" Z | Z
R  -> "<" | ">" | "<=" | ">=" | "==" | "!="
Z  -> ID | NUM | CHARCONST | "(" E ")"
```

## 2. LL(1) Grammar

This predictive grammar uses same short names and explicit epsilon rules.
Quoted items are literal source symbols or keywords.

```text
P  -> D P | ε
D  -> T ID X | "void" ID F
X  -> F | V
F  -> "(" A ")" B
A  -> L | "void" | ε
L  -> T ID Q
Q  -> "," T ID Q | ε
V  -> C ";"
C  -> "," ID C | ε
T  -> "int" | "char"
B  -> "{" G H "}"
G  -> T ID V G | ε
H  -> S H | ε
S  -> ID "=" E ";"
    | "if" "(" E ")" B O
    | "while" "(" E ")" B
    | "in" "(" ID ")" ";"
    | "out" "(" E ")" ";"
    | "return" U
    | B
    | ";"
O  -> "else" B | ε
U  -> E ";" | ";"
E  -> N W
W  -> R N | ε
N  -> M Y
Y  -> "+" M Y | "-" M Y | ε
M  -> Z J
J  -> "*" Z J | "/" Z J | "%" Z J | ε
R  -> "<" | ">" | "<=" | ">=" | "==" | "!="
Z  -> ID | NUM | CHARCONST | "(" E ")"
```

`else-part` is LL(1) here because every `if` branch is a compound statement.
The grammar therefore has no dangling-`else` conflict. The parser requires
braces around `if` and `while` bodies, as in `examples/hello.cmm`.

## 3. Parser Build and Run

```sh
make
./build/cmmc source.cmm
```

The parser reports syntax or lexical errors and exits nonzero. On success it
writes `tac.txt` (three-address code) and `symtab.txt` (symbol table) in the
current directory. It emits code during parsing and does not build an AST or
perform type checking. `examples/error-lexical.cmm` contains an intentional
`@`, so the parser rejects it.

## 4. Scope

The parser does not accept comments, unary operators, function-call
expressions, function prototypes, or unbraced `if`/`while` bodies. These are
outside the Phase 1 lexer/parser subset.
