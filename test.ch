fn malloc (size : int) -> (opaque) cffi {
    void *data = malloc(size * sizeof(int));
    ch_stk_push(&__istack, ch_valof_opaque(data));
    @return@
}

fn free (p : opaque) -> () cffi {
    free(p);
    @return@
}

fn ⊂ (i : int mem : opaque) -> (int opaque) cffi {
    int x = ((int*)mem)[i];
    ch_stk_push(&__istack, ch_valof_opaque(mem));
    ch_stk_push(&__istack, ch_valof_int(x));
    @return@
}

fn ⊂! (i : int val : int mem : opaque) -> (opaque) cffi {
    ((int*)mem)[i] = val;
    ch_stk_push(&__istack, ch_valof_opaque(mem));
    @return@
}

fn main () -> () {
↓                       vv Get 0   vv Get 1   vv Free    vv Print
→ 2 malloc 10 0 ⊂! 15 1 ⊂! 0 ⊂ ↕   1 ⊂ ↕      free       dbg
  ^        ^^      ^^ set 1 to `15`
  |        \-Set 0 to `10`
  Alloc 2 ints
}