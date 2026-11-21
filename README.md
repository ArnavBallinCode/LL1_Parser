# LL(1) Parser in C++

An LL(1) parser written from scratch in C++. It does not use any parser libraries. The program:

1. Defines an expression grammar.
2. Computes the **FIRST** and **FOLLOW** sets.
3. Builds the **LL(1) parsing table** and checks it for conflicts.
4. Parses input using a **stack** and prints every step.

**Author:** Arnav Angarkar (24BCS015)

## Grammar

```
E  -> T E'
E' -> + T E' | eps
T  -> F T'
T' -> * F T' | eps
F  -> ( E ) | id
```

## FIRST and FOLLOW

| Non-terminal | FIRST     | FOLLOW         |
|--------------|-----------|----------------|
| E            | { (, id } | { $, ) }       |
| E'           | { +, eps }| { $, ) }       |
| T            | { (, id } | { $, ), + }    |
| T'           | { *, eps }| { $, ), + }    |
| F            | { (, id } | { $, ), *, + } |

## Parsing table

|    | +          | *            | (         | )        | id        | $        |
|----|------------|--------------|-----------|----------|-----------|----------|
| E  |            |              | E -> T E' |          | E -> T E' |          |
| E' | E' -> + T E' |            |           | E' -> eps |          | E' -> eps |
| T  |            |              | T -> F T' |          | T -> F T' |          |
| T' | T' -> eps  | T' -> * F T' |           | T' -> eps |          | T' -> eps |
| F  |            |              | F -> ( E )|          | F -> id   |          |

The table has no conflicts, so the grammar is LL(1).

## Build and run

```bash
g++ ll1_parser.cpp -o ll1_parser
./ll1_parser
```

The program prints the grammar, the FIRST and FOLLOW sets, the parsing table, and step-by-step traces for three sample inputs. You can then type your own expressions, such as `a + b * c` or `(x+y)*z`. Type `exit` to quit.

Any name or number in the input is read as the token `id`.

## Example

```
 STACK                INPUT                 ACTION
 $ E                  id + id * id $        E -> T E'
 $ E' T               id + id * id $        T -> F T'
 ...
 $                    $                     ACCEPT
 Result: STRING ACCEPTED
```

## Project structure

| File | Description |
|------|-------------|
| `ll1_parser.cpp` | Parser source code |
| `report/LL1_Parser_Report.pdf` | 2-page project report |
| `report/LL1_Parser_Report.tex` | LaTeX source of the report |
