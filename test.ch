fn odd (n) -> (bool) {
→ ⇈ 0 = ? 1 - even 
        ↓
       '⊥
}

fn even (n : int) -> (bool) {
→ ⇈ 0 = ? 1 - odd
        ↓
        '⊤
}

fn main () -> () {
→ 4 even print 3 odd print 5 even print 10 odd print
}