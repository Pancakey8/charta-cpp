type pair (x : int y : char)

fn print-pair (p : pair) -> () {
→ pair.x str ", y = " & ↕ pair.y str ↻ ↕ & "pair { x = " ↕ & " }" & print
}

fn main () -> () {
→ 'a' 3 pair! ⇈ print-pair ↓
                           → pair.x 2 + pair.x! print-pair
}
