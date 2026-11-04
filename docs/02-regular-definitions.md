# Regular Definitions

*Assignment phase 2.*

Every token class with more than one possible lexeme has a regular definition.

```
digit       → 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
letter      → a | b | c | ... | z | A | B | C | ... | Z | _
id          → letter (letter | digit)*
num         → digit+
char        → any character except ' and newline
char_const  → ' char '
relop       → < | > | <= | >= | == | !=
assignop    → =  (distinct from relop via lookahead)
single_char → ; | , | ( | ) | { | } | + | - | * | / | %
```

## Keyword Handling Trick

Keywords are **not** separate regular definitions. Instead:

1. Scan identifier via `id` path: `(letter | _) (letter | digit | _)*`
2. After identifier is fully scanned (lookahead is not `letter`, `digit`, or `_`), call `lookupIdOrKeyword(lexeme)`
3. If lexeme matches a keyword, return the keyword TokenType; otherwise return ID token type

This means the regular definition for identifier accepts letter, digit, and underscore sequences after a valid first character. Keyword checking happens as a post-scan table lookup — one identifier path, multiple keyword results.

## Shorthand Reference (for diagram writing)

| Symbol | Regular Eq |
|---|---|
| `digit` | `0|1|2|3|4|5|6|7|8|9` |
| `letter` | `a|...|z|A|...|Z|_` |
| `id` | `letter (letter|digit)*` |
| `num` | `digit+` |
| `char` | any character except `'` and newline |
| `char_const` | `' char '` |
| `relop` | `<|>|<=|>=|==|!=` |
| `single_char` | `;|,|(|)|{|}|+|-|*|/|%` |