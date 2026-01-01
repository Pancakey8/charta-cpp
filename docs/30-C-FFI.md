# C FFI in Charta

Charta allows for the embedding of C code. If the keyword `cffi` is declared in
between the returns list and the start of the body, then the block of code is
interpreted plainly as C code.

Within the C code, `@return@` must be used over regular `return` to handle
necessary events.

``` c
fn hello () -> () cffi {
    printf("Hello, world!\n");
    @return@
}

fn main () -> () {
→ hello
}
```

**NOTE:** Above example depends on `printf` from `stdio.h`. As such, to compile
above code, `-cargs '-include "stdio.h"'` must be specified to ensure `printf`
is found.

## Taking parameters

In C FFI mode, arguments are popped from the stack and converted to named
variables automatically. To ensure a healthy conversion, name arguments after
valid C identifiers for FFI functions.

``` c
fn print-num (n : int) -> () cffi {
    printf("Number: %d\n", n);
    @return@
}
```

Parameter types are converted to C types as below:

| Charta Type    | C Type                                                                   |
|----------------|--------------------------------------------------------------------------|
| `int`          | `int`                                                                    |
| `float`        | `float`                                                                  |
| `char`         | `int` representing codepoint                                             |
| `bool`         | `char` but only `1` or `0`                                               |
| `string`       | `struct ch_string { char *data; size_t len; size_t size; }`              |
| `stack`        | Pointer to `struct ch_stack_node { ch_value val; ch_stack_node *next; }` |
| Type variables | Not supported explicitly                                                 |

**NOTE:** Since some values such as `ch_string` and `ch_stack_node*` are
dynamically allocated, `@return@` calls the destroy functions of these types. If
the values of these types are passed to a C function without copying, then
use-after-free bugs may occur.

**NOTE 2:** Documenting the internal API fully isn't a concern here, it is best
to read the [core header](../core/core.pre.h).

## Returning values

Any C FFI function has an implicitly defined variable
`struct ch_stack_node *__istack` containing its arguments. Since named
arguments are automatically popped, this stack is empty unless `...` is used.

To return values, `ch_stk_push` must be called to push any values that need to
be returned onto the `__istack`. The return process works as it does for regular
functions, where values are popped from the top and any remainder is destroyed.

`void ch_stk_push(struct ch_stack_node **, ch_value);` takes a `ch_value` which
may be constructed using one of the helpers, `ch_valof_int(int)`,
`ch_valof_float(float)`, `ch_valof_char(int)`, `ch_valof_bool(char)`,
`ch_valof_string(ch_string)`, `ch_valof_stack(struct ch_stack_node *)`.

An example of a standard fibonacci number implementation is given below.

``` c
fn fibo (n : int) -> (int) cffi {
    int a = 0, b = 1;
    for (int i = 0; i < n - 1; ++i) {
        int c = a + b;
        a = b;
        b = c;
    }
    ch_stk_push(&__istack, ch_valof_int(b));
    @return@
}

fn main () -> () {
→ 40 fibo print
}
```

## Non-standard returns

It is common for C libraries to hold handles, such as file handles or window
handles, for various interactions. To handle this, Charta has a special `opaque`
type. The `opaque` type is a special type that cannot be copied through
traditional means, such as the use of `⇈`. The `opaque` is also not freed when
popped, so the handling of `opaque` is up to the C code. `opaque` isn't produced
by any part of the stdlib, and it cannot be produced or manipulated outside an
FFI context directly.

``` c
fn malloc (size : int) -> (opaque) cffi {
    void *data = malloc(size * sizeof(int));
    ch_stk_push(&__istack, ch_valof_opaque(data));
    @return@
}

fn free (p : opaque) -> () cffi {
    free(p);
    @return@
}

fn ⊂ (i : int mem : opaque) -> (int opaque) cffi {
    int x = ((int*)mem)[i];
    ch_stk_push(&__istack, ch_valof_opaque(mem));
    ch_stk_push(&__istack, ch_valof_int(x));
    @return@
}

fn ⊂! (i : int val : int mem : opaque) -> (opaque) cffi {
    ((int*)mem)[i] = val;
    ch_stk_push(&__istack, ch_valof_opaque(mem));
    @return@
}

fn main () -> () {
↓                       vv Get 0   vv Get 1   vv Free    vv Print
→ 2 malloc 10 0 ⊂! 15 1 ⊂! 0 ⊂ ↕   1 ⊂ ↕      free       dbg
  ^        ^^      ^^ set 1 to `15`
  |        \-Set 0 to `10`
  Alloc 2 ints
}
```

## Calling Charta functions

Charta functions are always called by passing a `struct ch_stack_node **`, from
which the function pops its arguments. The functions always return a
`struct ch_stack_node *`, a stack object containing their return values.

Because C doesn't allow function names such as `+` or `⇈`, function names in
Charta are mangled. As a result, calling a Charta function requires the name of
the function to be wrapped in `@(` and `)@`.

``` c
fn call-dup (n : int) -> (int int) cffi {
    ch_stack_node *stack = ch_stk_new();
    ch_stk_push(&stack, ch_valof_int(n));
    ch_stack_node *result = @(⇈)@(&stack);
    ch_stk_append(&__istack, result);
    @return@
}

fn main () -> () {
→ 10 call-dup dbg
}
```

## Includes & C-specifics

C code may require external headers and linking. Any C compiler arguments may be
passed using `-cargs "<flags>"`. For example, one may use libm with `-cargs '-lm -include "math.h"'`.

Specifying includes however causes clutter and is potentially fragile, as such
`cimport "header.h"`, e.g. `cimport "math.h"`, may be used to specify required
headers. The location of `cimport` doesn't matter as it is always hoisted to the
top.
