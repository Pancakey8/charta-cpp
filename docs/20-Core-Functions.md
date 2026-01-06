# Core Functions

Core functions will be analysed in groups. 

## Operations affecting the stack

The functions below aren't practical on their own, as they only allow for
rudimentary manipulation and inspection of the stack.

| Name | ASCII  | Description                                                |
|:----:|:------:|:-----------------------------------------------------------|
| `⇈`  | `dup`  | Duplicates top value                                       |
| `⊼`  | `ovr`  | Duplicates second value to the top                         |
| `⊻`  | `tck`  | Duplicates first value below the second value              |
| `⩞`  | `pck`  | Duplicates third value to the top                          |
| `↕`  | `swp`  | Swaps the place of top two values                          |
| `↨`  | `swpd` | Swaps the place of the second and third values             |
| `↻`  | `rot`  | Moves the third value to the top                           |
| `↷`  | `rot-` | Moves the top value to the third position (inverse of `↻`) |
| `◌`  | `pop`  | Destroys the top value                                     |
| `⦵`  | `nip`  | Destroys the second value                                  |
| `≡`  | `dpt`  | Calculates the depth of the stack                          |

    
## Arithmetic operations 

All arithmetic functions take two numbers from the top of the stack, `int` or
`float`, and they return another number. `int` are promoted to `float` unless
two `int`s enter the function.

| Name | ASCII | Description    |
|:----:|:-----:|:---------------|
| `+`  |       | Addition       |
| `-`  |       | Subtraction    |
| `*`  |       | Multiplication |
| `/`  |       | Division       |
| `%`  |       | Modulo         |
    
## Equality and comparisons

The comparison functions only work on numbers, whereas (in)equality functions
work on any type.

| Name | ASCII | Description            |
|:----:|:-----:|:-----------------------|
| `=`  |       | Equality               |
| `≠`  | `!=`  | Inequality             |
| `<`  |       | Less than              |
| `≤`  | `<=`  | Less than or equals    |
| `>`  |       | Greater than           |
| `≥`  | `>=`  | Greater than or equals |

## Boolean operations

All operations below take and return booleans

| Name | ASCII | Description |
|:----:|:-----:|:------------|
| `∧`  | `&&`  | Logical and |
| `∨`  | `\|\|`  | Logical or  |
| `¬`  | `!`   | Logical not |

## Character operations

Characters have conversion methods defined between the codepoint number (as
integer) and the character representation.

| Name  | ASCII | Description                            |
|:-----:|:-----:|:---------------------------------------|
| `chr` |       | Codepoint (int) → character conversion |
| `ord` |       | Character → codepoint (int) conversion |

## Type operations

| Name | ASCII  | Description                                      |
|:----:|:------:|:-------------------------------------------------|
| `∈`  | `type` | Pushes the type-id of the top value on the stack |

This can be used for dynamic checks when working with generic code:

The following types can be tested against: `int`, `float`, `char`, `bool`,
`string`, `stack`.

``` rust
fn operate (x : #a) -> (int) {
→ ∈ char = ? ∈ int = ? 0
           ↓         ↓
          ord        2
                     *
}

fn main () -> () {
→ rand-bool ? 3   operate print
            ↓
           'a'
            →      ↑
}
```

In the above snippet, if `rand-bool`<sup>&ast;</sup> returns true, the code outputs `97`. Otherwise, it outputs `6`.

<sup>&ast;: <code>rand-bool</code> is implemented through C FFI for this example. Snippet won't work out of the box.</sup>

## Stack operations

Unlike the first section, functions in this section affect *stack objects*,
which are proper first-class values.

| Name | ASCII  | Description                                                        |
|:----:|:------:|:-------------------------------------------------------------------|
| `▭`  | `box`  | Moves the entirety of the current stack into a stack object.       |
| `⬚`  | `flat` | Expands stack object atop current stack.                           |
| `⤓`  | `ins`  | Inserts the top value into a stack on the second position          |
| `⊢`  | `fst`  | Retrieves the top value of a stack object                          |
| `⊢!` | `fst!` | Pops the top value of a stack object                               |
| `⊣`  | `lst`  | Retrieves the last value of a stack object                         |
| `⊣!` | `lst!` | Pops the last value of a stack object                              |
| `++` |        | Concatenates two stack objects.                                    |
| `⧺`  | `len`  | Calculates the length of a stack object                            |
| `↙`  | `take` | Takes N values from a stack object, moving into a new stack object |
| `↘`  | `drop` | Pops N values from a stack object                                  |
| `⇆`  | `rev`  | Reverses a stack object                                            |

Because it is more convenient to think of `5 ↙`, `"hello" ⤓`, ... as singular
operations, it is conventional for operations acting on an object (in this case,
stack objects) to take the object as its last parameter.

## String operations

Strings do not behave as stacks internally, and they get their own distinct
operations. The conversion between strings and stack objects is expensive and is
not implemented as a core function.

| Name  | ASCII  | Description                                                                             |
|:-----:|:------:|:----------------------------------------------------------------------------------------|
| `str` |        | Converts any value to a string representation                                           |
| `ℓ`   | `slen` | Retrieves the length of a string                                                        |
| `@`   |        | Gets Nth character on the string, indexed as codepoints                                 |
| `@!`  |        | Sets the Nth codepoint on the string to a character. `(idx : int ch : char s : string)` |
| `&`   |        | Concatenates two strings                                                                |
| `.`   |        | Pushes a character to the end of the string                                             |
| `.!`  |        | Pops a character from the end the string                                                |

## Function operations

These operations apply to function values.

| Name | ASCII    | Description                                           |
|:----:|:--------:|:------------------------------------------------------|
| `▷`  | `ap`     | Applies function to the stack.                        |
| `⟜`  | `tail`   | Applies function to the stack skipping the top value. |
| `⋄`  | `repeat` | Applies function to the stack N times.                |

## I/O

**TODO:** This section is lacking, but [C FFI](./30-C-FFI.md) should cover the missing pieces.

| Name    | ASCII | Description                                        |
|:-------:|:-----:|:---------------------------------------------------|
| `print` |       | Prints the top value, popping it                   |
| `dbg`   |       | Prints the entire stack, however doesn't modify it |

