# Charta

A 2D concatenative programming language.

```rs
fn fibo (n : int) -> (int) {
→ 0 1 ↻ 1 - ⇈ 0 > ? ◌
                  ↓
                  ↷
                  ⊼
                  ⊼
      ↑     ◌ ↻ + ←
}

fn main () -> () {
→ 40 fibo print
}

```

## Introduction

Charta is a concatenative language. Operations follow *Reverse Polish Notation*,
i.e. `2 + 3` is written as `2 3 +`. This also means that, if you want to apply
two functions back-to-back, `f(g(x))`, then you can naturally *compose* /
*concatenate* your functions `x g f`.

Charta is also a 2D language. Your program starts evaluating from the top-left
of a function and moves in the directions specified by the arrows `←, →, ↑, ↓`
until it exits the grid (and thus the function / program). You can change your
direction conditionally with `?`. If the previous expression evaluated to
`true`, that is if the top of your stack has `true`, then `?` rotates you to a
specified perpendicular direction. Using just `?` and directional arrows, you
can express control flow like loops and conditionals. Since control flow is
*only* written explicitly with arrows, code reads like a flow chart.

## Building the project

With GNU Make:
```
make all
```

With CMake:
```
cmake -Bbuild && cmake --build build
```

**NOTE:** Ensure that you preserve the file structure of `charta`, `core/core.h` and `libcore.a`!

```
.
├── charta
├── core
│   └── core.h
└── libcore.a
```

**TODO:** ↑ Make an `install` script for this step
