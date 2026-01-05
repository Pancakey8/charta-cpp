type vec2 (x : float y : float)

fn v+ (v : vec2 w : vec2) -> (vec2) {
→ ≍ ⟜ vec2.y ↻ + ↷ ≍ ⟜ vec2.x ↻ + ⦵ ⦵ vec2!
  ↓                ↓
vec2.y           vec2.x
}

fn vec2.print (v : vec2) -> () {
→ vec2.x str ", " & ↕ vec2.y str ⦵ & ")" & "(" ↕ & print
}

fn main () -> () {
↓
"Expected: (7.5, 2.0)"
print
→ 3.5 5.5 vec2! -1.5 2.0 vec2! v+ vec2.print
  ^ y ^ x        ^ y ^ x
}
