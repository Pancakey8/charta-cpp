fn ▭3 (a : #a b : #b c : #c) -> (stack) {
→ ▭
}

cimport "stdio.h"
cimport "stdlib.h"
cimport "string.h"

fn ∋ (x : string) -> () cffi {
   printf("%s", x.data);
   @return@
}

fn ↵ () -> (string) cffi {
   char *data = NULL;
   size_t sz = 0;
   ssize_t len;
   if ((len = getline(&data, &sz, stdin)) == -1) {
      free(data);
      printf("Exitting...\n");
      exit(0);
   }
   ch_string str;
   str.data = data;
   str.len = len - 1;
   str.data[str.len] = '\0';
   str.size = sz;
   ch_stk_push(&__istack, ch_valof_string(str));
   @return@
}

fn atoi (s : string) -> (int) cffi {
   ch_stk_push(&__istack, ch_valof_int(atoi(s.data)));
   @return@
}

fn cmd (s : string) -> ([string]) {
→ 0 "" ▭ ≍ ↕ ⊣ ℓ ⦵ ↨ ⋄ ⊢! ⦵ ▭ ⇆
         ↓
         ⬚         → 1 + "" ▭3
         ↻         ↻
         ⩞         ◌
         @         ↑
         → ⇈ ' ' = ? ↨ . ↻ 1 + ↕ ▭3
}

fn cmd/help (command : [string]) -> () {
↓
◌
"Usage:"
""
"  - help"
"  Displays this message"
""
"  - add <entry>"
"  Adds ENTRY to your list"
""
"  - list"
"  Lists all entries"
""
"  - remove <n>"
"  Removes Nth entry"
→ ▭ ⇆ ≍ ↕ ⧺ ↨ ⋄
      ↓
      ⊢!
      print
}

fn cmd/add (command : [string] list : [string]) -> ([string]) {
→ ⇆ "" ≍ ↻ ⧺ ↨ ⋄ ◌ ⤓
       ↓
       ⊢!
       " "
       &
       ↻
       &
       ↕
}

fn cmd/list (command : [string] list : [string]) -> ([string]) {
→ ◌ ⇈ ⇆ 0 ≍ ↻ ⧺ ↨ ⋄ ↻
          ↓
          ↕
          1
          +
          → ⇈ str ∋ ". " ∋ ↕ ⊢! print
}

fn cmd/remove (command : [string] list : [string]) -> ([string]) {
→ ⧺ 0 = ? ⊢! atoi ⦵ ⇈ 0 >  ? "index ≤ 0 not allowed" print ↕
        ↓                  ↓
"'remove' expected index"  → ⇈ ↻ ⧺ ↻ ≥ ? "index > list length not allowed" print
        print                          ↓
        ◌                              ⇆
                                       ↕
                                       1
                                       -
                                       → ↙ ↕ ⊢! ◌ ++ ⇆
}

fn main () -> () {
↓    ↓                              ←
→ ▭ "> " ∋ ↵ cmd ⊢! ↓
                    ⇈
                    "help"
                    =
                    ?→ ◌ cmd/help   ↑
                    ⇈
                    "add"
                    =
                    ?→ ◌ cmd/add    ↑
                    ⇈
                    "list"
                    =
                    ?→ ◌ cmd/list   ↑
                    ⇈
                    "remove"
                    =
                    ?→ ◌ cmd/remove ↑
                    ◌
                    ◌
         "Unknown command\n"
         "See ~help~"
                    → & print       ↑ 
}
