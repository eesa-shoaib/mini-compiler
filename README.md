# cmmc — C-- Compiler Front-End

A lexer, recursive-descent parser and virtual machine for **C--**, a small
C-like teaching language (`.cmm`). It tokenises source, validates the full
grammar, reports line-numbered lexical and syntax errors, emits three-address
code (TAC) plus a symbol table, and can execute that TAC.

## Layout

```text
.
├── Makefile
├── README.md
├── src/
│   ├── main.cpp       # entry point: CLI, parse, write outputs, optional run
│   ├── lexer.hpp
│   ├── lexer.cpp      # state-machine lexer
│   ├── parser.hpp
│   ├── parser.cpp     # recursive-descent parser
│   ├── codegen.hpp
│   ├── codegen.cpp    # TAC and symbol-table emission
│   ├── vm.hpp
│   └── vm.cpp         # virtual machine for the TAC
├── examples/
│   ├── hello.cmm          # valid program
│   ├── functions.cmm      # function calls with parameters/return values
│   ├── factorial.cmm      # recursive function
│   ├── error-lexical.cmm  # invalid character
│   └── error-syntax.cmm   # unbraced if body
└── docs/                  # spec, regexes, diagrams, grammar
```

## Build and run

```sh
make
./build/cmmc examples/hello.cmm        # Parsing successful.
./build/cmmc --run examples/factorial.cmm   # compile, then execute the TAC
```

Or use the helper script, which builds first:

```sh
./run.sh                          # parses examples/hello.cmm
./run.sh path/to/file.cmm         # parses a specific file
./run.sh file.cmm --run           # parses and executes the TAC
./run.sh file.cmm --tokens        # dumps the token stream
```

Every successful run writes two files in the current directory:

- `tac.txt` — three-address code
- `symtab.txt` — symbol table (name, kind, type, address, initial value)

Example `tac.txt`:

```text
func add
param a
param b
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

`int` / `char` / `void`, globals, functions with parameters and return values,
function calls (including recursion), assignment, `if`/`else`, `while`,
`in`/`out`, `return`, arithmetic (`+ - * / %`), relational operators, integer
and character constants.

Not supported: comments, unary operators, prototypes, unbraced `if`/`while`
bodies, AST construction, type checking, and native code generation. (Control
statements nest naturally because the parser is recursive descent; the course
just doesn't require handling that case.) The symbol table records declared
identifiers (functions, parameters, variables); compiler temporaries are only
named in the TAC.

## Virtual machine

`--run` executes the generated TAC with the built-in `VirtualMachine`.

| Instruction | Meaning |
|---|---|
| `func` / `endfunc` | function boundaries |
| `param x` | formal parameter declaration |
| `L:` | jump target |
| `goto L` | unconditional jump |
| `ifFalse x goto L` | jump when `x` is zero |
| `x = y` | copy, constant or character constant |
| `x = y op z` | arithmetic or relational operation |
| `in x` | read an integer from stdin |
| `out x` | print value (as a character for `char` variables) |
| `push x` | evaluate an actual argument |
| `call f, n` | call, discard the result |
| `t = call f, n` | call, keep the result in `t` |
| `return [x]` | return from the function |

Memory starts at zero. Each call gets fresh locals bound to its parameters, so
recursive functions work.

```text
$ printf '5\n' | ./build/cmmc --run examples/factorial.cmm
Parsing successful.
TAC written to tac.txt
Symbol table written to symtab.txt
120
```

Runtime errors (division by zero, unknown function or label, wrong argument
count, missing input) are reported on stderr and exit non-zero. Without `--run`
the compiler only writes `tac.txt` and `symtab.txt`, exactly as before.

## Docs

1. [Language spec](docs/01-language-spec.md)
2. [Regular definitions](docs/02-regular-definitions.md)
3. [Transition diagrams](docs/03-transition-diagrams.md)
4. [Lexer implementation](docs/04-lexer-implementation.md)
5. [Grammar and parser](docs/05-grammar-and-parser.md)

# Small commit #1 – added marker comment to README
# commit 1 – marker in README
