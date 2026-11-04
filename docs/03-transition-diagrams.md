# Transition Diagrams

*Assignment phase 3. One combined diagram with numbered states `S0` through
`S18`.*

## State-to-Code Mapping

The implementation has exactly one `switch` case for every state shown below.

| State | Meaning | Accepting |
|---|---|---|
| `S0` | Start and character dispatch | No |
| `S1` | Identifier loop | No |
| `S2` | Number loop | No |
| `S3` | `<` lookahead | No |
| `S4` | `<=` | Yes |
| `S5` | `<` after retract | Yes |
| `S6` | `>` lookahead | No |
| `S7` | `>=` | Yes |
| `S8` | `>` after retract | Yes |
| `S9` | `=` lookahead | No |
| `S10` | `==` | Yes |
| `S11` | `=` after retract | Yes |
| `S12` | `!` lookahead | No |
| `S13` | `!=` | Yes |
| `S14` | Opening character quote | No |
| `S15` | Character body | No |
| `S16` | Complete character constant | Yes |
| `S17` | Single-character token | Yes |
| `S18` | Error token and recovery | Yes |

## Relational and Assignment Families

```text
S0 --<--> S3 --=--> S4*(<=)       [emit LE]
S3 --other--> retract, S5*(<)     [emit LT]

S0 --> S6 --=--> S7*(>=)          [emit GE]
S6 --other--> retract, S8*(>)     [emit GT]

S0 --=--> S9 --=--> S10*(==)      [emit EQ]
S9 --other--> retract, S11*(=)    [emit ASSIGN]

S0 --!--> S12 --=--> S13*(!=)     [emit NE]
S12 --other--> S18*(ERROR)        [standalone ! is invalid]
```

## Identifier and Keyword Family

```text
S0 --letter or _--> S1
S1 --letter, digit, or _--> S1
S1 --other--> retract, accept, lookup keyword table
   ├─ keyword lexeme → emit KW_*
   └─ other lexeme   → emit ID
```

`S1` scans the complete identifier. The first non-identifier character is
retracted so it can begin the next token.

## Number Family

```text
S0 --digit--> S2
S2 --digit--> S2
S2 --other--> retract, accept, emit NUM
```

## Character-Constant Family

```text
S0 --'--> S14
S14 --any non-quote, non-newline character--> S15
S15 --'--> S16*(CHARCONST)
S14/S15 --invalid, newline, or EOF--> S18*(ERROR)
```

The complete lexeme, including both quotes, is preserved. A malformed
character literal enters `S18`, then consumes input through its closing quote
or line end before continuing.

## Punctuation and Single-Character Operators

The following characters transition directly from `S0` to `S17`:

```text
; , ( ) { } + - * / %
```

`S17` selects the token type from the character value.

## Error State

Any unsupported character transitions from `S0` to `S18`. The lexer emits an
`ERROR` token and continues scanning. Standalone `!` also enters `S18`.

## State-Numbering Convention

Every state maps 1:1 to a `switch` case in `src/lexer.cpp`. Accepting states are
marked with `*`. `S99_DONE` is not used.

## Next Phase

Phase 4 maps these states to the lexer implementation with a `Token` struct,
keyword table, retract support, line tracking, and driver loop.
