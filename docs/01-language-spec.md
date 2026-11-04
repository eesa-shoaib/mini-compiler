# Language Spec (C-- Subset)

*Assignment phase 1.*

## Token Categories

| Category | Examples |
|---|---|
| Keywords | `int`, `char`, `if`, `else`, `while`, `in`, `out` (optionally `return`, `void`) |
| Identifiers | `(letter | _)(letter | digit | _)*` |
| Integer Constants | `digit+` (e.g. `42`, `0`) |
| Character Constants | `' character '` (e.g. `'a'`, `'7'`, `' '`) |
| Operators | `=`, `+`, `-`, `*`, `/`, `%`, `<`, `>`, `<=`, `>=`, `==`, `!=` |
| Punctuation/Delimiters | `;`, `,`, `(`, `)`, `{`, `}` |
| Error/Unrecognized | Any character not covered above |

## Scope Decisions (for report)

1. **Comments** — `//` **not supported** in Phase 1. (Can be added later.)
2. **Identifier max length** — No hard limit; handled by buffer size in implementation.
3. **Two-character operator lookahead** — `<`, `>`, `=`, and `!` use lookahead for `<=`, `>=`, `==`, and `!=`. If the second character is absent, retract and emit `<`, `>`, or `=`.
4. **Whitespace** — Skipped, never emitted as a token.
5. **Case sensitivity** — Identifiers and keywords are case-sensitive (C convention).
6. **Character literals** — Exactly one non-quote, non-newline character is allowed between single quotes. The complete lexeme, including quotes, is preserved.
7. **Malformed character recovery** — Emit one `ERROR` token, consume the rest of the malformed literal through its closing quote or line end, then continue.
8. **Error recovery** — On an unrecognized character: emit `ERROR` token with that character, then advance one position and continue.

## Grammar Sketch (informal)

This sketch is preliminary. The implemented parser subset, including its
block-body rule for `if` and `while`, is defined in
[`05-grammar-and-parser.md`](05-grammar-and-parser.md).

```
Program → Stmt*
Stmt → Assign | If | While | FunDecl | Empty
Assign → ID = Expr
If → `if` '(' Expr ')' Stmt (`else` Stmt)?
While → `while` '(' Expr ')' Stmt
Expr → Term (('+'|'-') Term)*
Term → Factor (('*'|'/') Factor)*
Factor → NUM | ID | CHARCONST | '(' Expr ')'
```

## Keyword Table (reserved)

```
int → TYPE_INT
char → TYPE_CHAR
if → KW_IF
else → KW_ELSE
while → KW_WHILE
in → KW_IN
out → KW_OUT
(return optional) → KW_RETURN
(void optional) → KW_VOID
```

## Next Phase

Phase 2: Write regular definitions for every token class with multiple lexemes.
Phase 3: Draw transition diagrams (states, accepting states, retract marks).
Phase 4: Implement code mirroring diagram states `S0` through `S18` 1:1.