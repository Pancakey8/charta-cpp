type foo (val : int next : [foo])

fn ¿next (x : foo) -> (bool foo) {
→ foo.next ⧺ 0 ≠ ↨
}

fn nil () -> ([foo]) {
→ ▭
}

fn · (f : foo) -> ([foo]) {
→ ▭
}

fn show-foo (x : foo) -> (string) {
→ foo.val str ↕ ¿next ? ◌ 
                      ↓
                      foo.next
                      ⊢!
                      ⦵
                      ⦵
                      show-foo
                      " -> "
                      ↕
                      &
                      &
}

fn main () -> () {
→ nil 2 foo! · 3 foo! · 4 foo! ⇈ show-foo print · 5 foo! show-foo print
}

