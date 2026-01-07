type 𝟚 (k : string v : int)

fn freq! (word : string freq-table : [𝟚]) -> (... 𝟚) {
↓                        ←
→ ↕ ∘ ? ⊢! ↨ 𝟚.k ⩞ = ? ↷ ↑
      ↓              ↓
      ◌             𝟚.v
      1              1
      ↕              +
      𝟚!             → 𝟚.v! ⦵ ↕ ⬚
}

fn put-tbl (table : [𝟚]) -> () {
→ ≍ ↕ ⧺ ↨ ⋄
  ↓
  → ⊢! 𝟚.v str ↕ 𝟚.k ": " & ↻ & print ◌
}

fn ¿brk (c : char) -> (bool char) {
↓             ↓              ←
                             ↑
→ " \n.,;!:?" ℓ 0 = ? .! ⩞ ≠ ? ◌ '⊤
                    ↓
                    ◌
                   '⊥
}

fn str⇆ (s : string) -> (string) {
→ ≍ ↕ ℓ ↨ ⋄ ◌ ▭ ⇆ "" ≍ ↻ ⧺ ↨ ⋄ ◌
  ↓                  ↓
  .!                 ⊢!
  ↕                  ↨
                     .
                     ↕
}

fn words (s : string) -> ([𝟚]) {
↓                           → str⇆ freq!
                            ↑
→ ▭ ⊢! "" ≍ ↻ ℓ ↨ ⋄ ◌ ℓ 0 ≠ ? ◌
          ↓
          .!           
          ¿brk         
          ?→ ◌ ↷ ℓ 0 = ? str⇆ freq! "" ↻
          ↨            ↓
          .            ↻
          ↕
}

fn fread (fp : string) -> (string) cffi {
   FILE *f = fopen(fp.data, "r");
   if (!f) goto error;
   if (fseek(f, 0, SEEK_END) == -1) goto error;
   ssize_t len;
   if ((len = ftell(f)) == -1) goto error;
   rewind(f);
   ch_string str = ch_str_alloc(len);
   str.len = len;
   if (fread(str.data, 1, len, f) != len) {
      ch_str_delete(&str);
      goto error;
   }
   fclose(f);
   ch_stk_push(&__istack, ch_valof_string(str));
   @return@
   error:
   fclose(f);
   puts("Failed to read file\n");
   exit(1);
}

fn main () -> () {
→ "./bee-movie.txt" fread words put-tbl
}
