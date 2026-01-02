fn numeral-table () -> (stack) {
→ "I" 1 "IV" 4 "V" 5 "IX" 9 "X" 10 "XL" 40 "L" 50 "XC" 90 "C" 100 "CD" 400 "D" 500 "CM" 900 "M" 1000 ▭
}

fn sym! (tbl : stack s : string n : int) -> (string stack string int) {
→ ⩞ ↕ ⊢ ↻ > ? ↻ ↕ ⊢ ↻ ↕ - ↷ ⊢! ↕ ⊢ ↕ ↻ ⤓ ↕
            ↓
            2
            ↘
            ""
}

fn roman (n : int) -> (string) {
↓
""
numeral-table
⧺               ←
0
>
?→ sym! ↻ ↕ & ↕ ↑
◌
}

cimport "stdio.h"

fn input () -> (int) cffi {
   int n;
   if (scanf("%d", &n) == EOF) {
      n = 0;
   }
   ch_stk_push(&__istack, ch_valof_int(n));
   @return@
}

fn main () -> () {
→ input roman print
}
