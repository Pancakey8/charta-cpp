fn fibo (x : int) -> (int) {
→ dup 0 = ?  dup 1 = ?  dup 1 - fibo swp 2 - fibo +
          ↓         v|
}

fn main () -> () {
-> 10 fibo print
}
