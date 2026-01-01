fn repeat (n : int f : function s : stack) -> (stack) {
↓
⇈                     ←
0
>                     ↷
?→ 1 - ↻ ⩞ ↕ ⊣! ↻ ▷ ⤓ ↑
◌            ^^ Inefficient
◌
}

fn map (f : function s : stack) -> (stack) {
↕ ⧺ ↻ ↕ repeat
}

cimport "stdio.h"

fn thing (f : function) -> () cffi {
   ch_stack_node *stk = ch_stk_new();
   for (int i = 0; i < 10; ++i) {
      ch_stk_push(&stk, ch_valof_int(i));
   }
   ch_stack_node *ret = f(&stk);
   ch_stack_node *s = ret;
   while (s != NULL) {
      printf("Thing: %d\n", s->val.value.i);
      s = s->next;
   }
   ch_stk_delete(&ret);
   @return@
}

fn main2 () -> () {
→ 1 2 3 4 5 ▭ ≍ map print
              ↓
              1
              +
}

fn main () -> () {
→ ≍ thing main2
  ↓
  ▭
  → ⧺ 0 > ? ◌
          ↓
          ⊢!
          1
    ↑ ↕ + ←
}