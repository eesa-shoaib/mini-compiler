# cmmc — C-- Compiler Front-End

A lexer and recursive-descent parser for **C--**, a small C-like teaching
language (`.cmm`). It tokenises source, validates the full grammar, reports
line-numbered lexical and syntax errors, and emits three-address code (TAC)
plus a symbol table.

## Layout

```text
.
├── Makefile
├── README.md
├── src/
│   ├── main.cpp       # entry point: CLI, parse, write outputs
│   ├── lexer.hpp
│   ├── lexer.cpp      # state-machine lexer
│   ├── parser.hpp
│   ├── parser.cpp     # recursive-descent parser
│   ├── codegen.hpp
│   └── codegen.cpp    # TAC and symbol-table emission
├── examples/
│   ├── hello.cmm          # valid program
│   ├── error-lexical.cmm  # invalid character
│   └── error-syntax.cmm   # unbraced if body
└── docs/                  # spec, regexes, diagrams, grammar
```

## Build and run

```sh
make
./build/cmmc examples/hello.cmm   # Parsing successful.
```

Or use the helper script, which builds first:

```sh
./run.sh                          # parses examples/hello.cmm
./run.sh path/to/file.cmm         # parses a specific file
./run.sh file.cmm --tokens        # dumps the token stream
```

Every successful run writes two files in the current directory:

- `tac.txt` — three-address code
- `symtab.txt` — symbol table (name, kind, type, address, initial value)

Example `tac.txt`:

```text
func add
t1 = a + b
sum = t1
return sum
endfunc add
...
```

Example `symtab.txt`:

```text
Name            Kind        Type    Address   Initial
----------------------------------------------------------
add             function    int     1000      -
a               parameter   int     1004      -
sum             variable    int     1012      -
```

Errors exit non-zero:

```text
Lexical error at line 4: unrecognized token '@'
Syntax error at line 6: expected '{' near 'x'
```

## Language

`int` / `char` / `void`, globals, functions with parameters, assignment,
`if`/`else`, `while`, `in`/`out`, `return`, arithmetic (`+ - * / %`),
relational operators, integer and character constants.

Not supported: comments, unary operators, function calls, prototypes, unbraced
`if`/`while` bodies, nested control-flow requirements, AST construction, type
checking, and native code generation. The symbol table records declared
identifiers (functions, parameters, variables); compiler temporaries are only
named in the TAC.

## Docs

1. [Language spec](docs/01-language-spec.md)
2. [Regular definitions](docs/02-regular-definitions.md)
3. [Transition diagrams](docs/03-transition-diagrams.md)
4. [Lexer implementation](docs/04-lexer-implementation.md)
5. [Grammar and parser](docs/05-grammar-and-parser.md)
