fn foo (x : int) -> (... int) {
→ ⇈ 0 =  ?  ↓
         ↓  ⇈
  ↑ - 1     ←
}

fn bar (x : int) -> (... int) {
→ ⇈ 0 =  ?  ↓
         ↓  print
  ↑         ←
}

fn main () -> () {
→ 5 foo print 10 bar print
}

