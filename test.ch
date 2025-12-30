fn call-dup (n : int) -> (int int) cffi {
    ch_stack_node *stack = ch_stk_new();
    ch_stk_push(&stack, ch_valof_int(n));
    ch_stack_node *result = @(⇈)@(&stack);
    ch_stk_append(&__istack, result);
    @return@
}

fn main () -> () {
→ 10 call-dup dbg
}