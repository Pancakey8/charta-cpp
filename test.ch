fn line-count (s : string) -> (int) {
↓                → -1 ↓  Handle trailing newline
                 ↑
→ ℓ 1 - @ '\n' = ? 0  → ≍ ↻ ℓ ↨ ⋄ ◌ 1 +
                        ↓        Line count = Newlines + 1
                        .!
                        '\n'  Count newlines
                        =
                        ?→ ↕ 1 + ↕
}

fn read (path : string) -> (string) cffi {
   // No error handling, don't beat me ^-^
   FILE *f = fopen(path.data, "r");
   fseek(f, 0, SEEK_END);
   long size = ftell(f);
   rewind(f);
   ch_string str = ch_str_alloc(size);
   fread(str.data, 1, size, f);
   fclose(f);
   str.data[size] = '\0';
   str.len = size;
   ch_stk_push(&__istack, ch_valof_string(str));
   @return@
}

fn main () -> () {
→ "./bee-movie.txt" read line-count print
}
