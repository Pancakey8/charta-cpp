# Getting Started with Charta

## Concatenative programming

Charta is stack-based and concatenative. The two ideas are distinct but closely related.

In stack-based programming languages, operations take input from and output to a
stack object. In Charta, there exists only a single stack that holds the various
fundamental types of data.

In concatenative languages, function calls and composition are implicit. For example, in Charta

``` rust
2 3 +
```

calls a function `+` on the two parameters `2` and `3`. This is achieved, in
Charta, through the use of the stack. However, a stack isn't necessary to
achieve concatenative programming.

For further reading: [Concatenative language](https://concatenative.org/wiki/view/Concatenative%20language)

## 2D code

2D-ness of code doesn't affect the execution of code. At any point, only one
instruction is executed, as with other languages. However, 2D code requires one
to consider:

- Direction of execution
- Direction of control flow

The direction of execution refers to the direction in which code is read. In
most languages, the default behaviour is to read left-to-right unless operator
precedence dictates otherwise, and lines are read top-to-bottom. In Charta, code
is organised into functions, where each function contains a 2D grid. Execution
always starts at the top-left corner.

``` rust
fn main () -> () {
@ ← Start here


↓ Not here           Not here  ↓
@                              @
}
```

From the starting point, code always follows the direction of the four
directional arrows `↑ ← ↓ →`. ASCII variants of these symbols are `|^ <- |v ->`.
Note that up and down arrows can also be written as `^| v|`. For up and
down arrows, the characters `^` and `v` are treated as if they don't exist, so
the position of the arrow is dictated by `|`.

``` rust
fn main () -> () {
→ 1    2 ↓
         3
         
         4 
6     5  ←
}
```

Execution hits all 6 numbers in order in the example above. After reaching `6`,
moving left brings us outside the grid, as such execution returns from function
`main` and terminates the program.

The direction of control flow refers to the direction rules when control flow is
introduced. There are two cases to consider here: Loops and branches.

Loops in Charta are traversed once. Code is evaluated normally until it hits a
point it has already been before. Once this happens, a loop is formed. At the
point of the loop, execution follows code as it had followed it beforehand. This
means that, even without an explicit arrow to redirect execution, the execution
will change directions.

``` rust
fn main () -> () {
→ 10 ⇈ print ↓
     ↑   - 1 ←
}
```

In the above snippet, `⇈` (ASCII: `dup`) is a function that duplicates top
element on the stack. As such, the code snippet first pushes a value `10` on the
stack, then each time `⇈` is reached the code prints a copy of the top value (as
`print` pops the top value) and then it decrements the top value (`1 -`). Even
though the direction of execution is in the up direction before `⇈` is reached a
second time, direction is assumed to be the right direction at `⇈` because that
is how `⇈` was traversed the first time around.

Branches behave different to this. First, to introduce branches, they are
denoted with `?` followed by a direction arrow. `?` is a special symbol that
takes a boolean from the top of the stack, and rotates the direction of
execution only if the boolean is true. Otherwise, `?` does nothing more, though
the value is popped either way. The 4 valid ways to construct `?`, one for each
direction, are exemplified below (examples are assumed to be function bodies,
read starting at top-left):

``` rust
→ 2 3 = ? false-case
        ↓
       true-case
```

``` rust
↓      true-case
        ↑
→ 2 3 = ? false-case
```

``` rust
↓
2
3
=
?→ true-case
false-case
```

``` rust
→          ↓
           2
           3
           =
true-case ←?
           false-case
```

Now as for the direction of control flow, branches assume the `false-case` to be
the "main branch". As such, if the true branch merges into the false branch, it
will be redirected to the direction of the false branch. However, if the false
branch is redirected into the true branch, it will keep its direction.

This behaviour can be observed by comparing the two functions below:

``` rust
fn false-to-true () -> () {
→ '⊥ ? 3       ↓
     ↓
     1
     print     ←
     2
     print
}

fn true-to-false () -> () {
→ '⊤ ? 1 print 2 print
     ↓
     3
     →    ↑
}
```

`false-to-true` enters the false branch, pushes `3` on the stack, prints it by
using the `print` of the true branch; however, execution isn't redirected, and
`2 print` isn't evaluated.

`true-to-false` enters the true branch, pushes `3` on the stack, prints it by
using the `print` of the false branch. This time, execution *is* redirected and
we also evaluate `2 print`.

## Basics of values & types

In Charta, the following types exist concretely and also have literals:

``` rust
int -> 5 , -12
float -> 2.718, 3.14
char -> 'a', 'ü', '😃'
bool -> '⊤, '⊥ (ASCII: 'T, 'F)
string -> "Hello world!"
```

Stacks can also be stored as objects, but they are constructed indirectly. The
function `▭` (`box`) stores all values on the stack into one *stack object*.

``` rust
fn main () -> () {
→ 1 2 3 ▭ print
}
```

As with stacks, function values are constructed indirectly. The operator `≍`
(`~~`) takes a perpendicular branch and pushes it as a function to the stack, to
later be called with `▷` (`ap`).

Above snippet outputs `[3, 2, 1]`. For stack and stack-related notations, the
top of the stack is always written first. We will also observe this when talking
about functions, as function parameters & returns are written in the order of
top-to-bottom.

Stacks have two types in the context of type signatures:

- `[T]` i.e. `[int], [char], ...` refers to a homogeneous stack.
- `stack` refers to a heterogeneous stack.

The distinction between the two signatures is only done in the context of
*arguments & returns*, and a function is free to turn a homogeneous stack into a
heterogeneous one by pushing different types into it. The distinction only helps
define functions more expressively.

There is one final type to consider, which is the *type variable*. Any valid
identifier, prefixed with `#` is considered a type variable in a function
context. A type variable helps to express operations that are independent of
type, such as `⇈`. Though functions have yet to be explained, the signature of
`⇈` may aid in understanding type variables.

``` rust
fn ⇈ (x : #a) -> (#a #a)
```

## Functions

In Charta, functions are defined as below

``` rust
'fn' <name> '(' [<name> : <type>]* ['...']? ')' '->' '(' [<type>]* ['...' <type>]? ')' '{'
<body>
'}'
```

`<name>` consists of any valid unicode character barring spaces. `<name>` cannot
contain any reserved string, which includes `'`, `?`, ASCII arrows only if exact
match (i.e. `>` is allowed but not `->`), unicode arrows, and the three
parenthesis types `[](){}`.

`<type>` can be the name of any of the types listed above, as in `string`,
`stack`, `int`; it can also be a homogeneous stack type such as `[int]`, or it
can be a type variable such as `#a`.

The function pops its arguments from the stack, with the first argument being
take from the *top* of the stack (See previous section, the top of the stack is
always written first). If the argument list terminates with `...`, then the
remaining elements of the stack are popped into one *stack object*. Though
argument names are given, they are not used<sup>&ast;</sup> and only serve to
document code. Arguments are also **not separated with commas**, neither are the
returns, unlike other languages.

The return list denotes the state of the *top* of the stack once the function
terminates. If the function has any extra values, they are destroyed. That is,
if the function terminates with a stack of `[3, 2, 1]`, but it only has to
return `(int)`, then `2` and `1` are destroyed. If the return list terminates
with `... <type>`, then instead of destroying the remaining values, the function
attempts to return the rest of the stack as a homogeneous *stack object* of
`<type>`s. Unlike homogeneous stacks, returning a heterogeneous stack isn't done
implicitly; it must be specified as `stack` in the return list and must be
constructed explicitly if desired.

Function values constructed with `≍` do not have to specify an argument or
return list, as they operate on the entire stack. Function values can be
reasoned about as inlined operations.

## Next Step

The language doesn't have more than writing and composing together
functions. With the knowledge in this document, simple programs can already be
written, so long as one knows the builtin functions. As such, the next document talks about
[Core Functions](./20-Core-Functions.md).

Since the [Core Functions](./20-Core-Functions.md) document serves as a
cheatsheet, reading [Examples](../examples/) alongside it will be a better
learning experience.
