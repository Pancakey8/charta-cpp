# Types

The builtin types are summarized below:

- `int` representing integer values.
- `float` representing floating point values.
- `bool` is either a `'⊤` or `'⊥` for true and false respectively.
- `char` is a full UTF8 character.
- `string` is an UTF8 string, all string methods work assuming UTF8.
- `stack` or `[type]` represent stack objects. The stricter `[type]` variant is
  only used for correctness & static checks, and both objects are heterogeneous.
- `opaque` is produced only in `cffi` code. It cannot be copied with `⇈` style
  functions. It has to be explicitly freed.
- `function` refers to function values. These can be passed as parameter to
  functions like `⟜`.
- `#symbol`, e.g. `#a`, `#x`, `#foo`; refers to type variables, which denote
  function parameters that act independent on underlying types.
  
Charta also allows user-defined types with the syntax below:

``` rs
type <name> ([<name> : <type>]*)
```

Here, `<type>` can be any type, barring type variables. Recursive types are allowed.

Types can be used to group meaningfully similar terms to simplify stack management.

``` rs
type vec2 (x : float y : float)
```

Defining the type `vec2` generates certain functions automatically,

| Function              | Purpose                                                                                |
| `vec2`                | Push type-id to stack for comparison with `∈`                                          |
| `vec2!`               | Constructor, pops left-most field first, consistent with other stack-related notations |
| `vec2.x` / `vec2.y`   | Generated per-field, getter (`fn vec2.x (obj : vec2) -> (float vec2)`)                 |
| `vec2.x!` / `vec2.y!` | Generated per-field, setter (`fn vec2.x! (new : float obj : vec2) -> (vec2)`)          |

Behind the scenes, each user-defined type generates its copy and delete
function. Delete is called when the value is popped, copy is called when a
copying operation is called (`⇈`, `⊼`, `⊢`, `⊣`, ...)

**TODO:** Extend this section
