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

fn ⊼ (a : #a b : #b) -> (#b #a #b) {
→ ↕ ⇈ ↻ ↻
}

fn fibo2 (n : int) -> (int) {
→ 0 1 ↻ 1 - ⇈ 0 > ? ◌
                  ↓
                  ↷
                  ⊼
                  ⊼
      ↑     ◌ ↻ + ←
}

fn main () -> () {
→ 40 fibo2 print
}

