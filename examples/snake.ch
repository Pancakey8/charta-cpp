cimport "termios.h"
cimport "unistd.h"
cimport "stdio.h"
cimport "stdlib.h"
cimport "time.h"

fn getchar () -> (int) cffi {
    char c = getchar();
    ch_stk_push(&__istack, ch_valof_int((int) c));
    @return@
}

fn unbuffer () -> (opaque) cffi {
    struct termios newt;
    struct termios *oldt = malloc(sizeof(struct termios));

    tcgetattr(STDIN_FILENO, oldt);
    newt = *oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch_stk_push(&__istack, ch_valof_opaque((void*) oldt));
    @return@
}

fn buffer (tios : opaque) -> () cffi {
    struct termios *old = (struct termios *)tios;
    tcsetattr(STDIN_FILENO, TCSANOW, old);
    free(old);
    @return@
}

fn ⇑ () -> (int) { 0 }
fn ⇓ () -> (int) { 1 }
fn ⇒ () -> (int) { 2 }
fn ⇐ () -> (int) { 3 }

fn W () -> (int) { 15 }
fn H () -> (int) { 15 }

fn † () -> (int) {
→ getchar 27 ≠ ? getchar 91 ≠ ? getchar 65 -
               ↓              ↓
              -1             -1
}

fn name (dir : int) -> (string) {
→ ⇈ ⇑ = ? ⇈ ⇓ = ? ⇈ ⇐ = ? ⇈ ⇒ = ? "Not a direction"
        ↓       ↓       ↓       ↓
     "Up"  "Down"  "Left" "Right"
}

fn draw (tail : [int] food_x : int food_y : int) -> ([int]) cffi {
   ch_stack_node *call = @(W)@(NULL);
   int W = ch_stk_pop(&call).value.i;
   call = @(H)@(NULL);
   int H = ch_stk_pop(&call).value.i;
   puts("\033[2J\033[H");
   for (int i = 0; i < H; ++i) {
      for (int j = 0; j < W; ++j) {
         ch_stack_node *s = tail;
         char is_tail = 0;
         while (s != NULL) {
             int x = s->val.value.i;
             s = s->next;
             int y = s->val.value.i;
             s = s->next;
             if (i == y && x == j) {
                is_tail = 1;
                break;
             }
         }
         if (is_tail) {
            putc('@', stdout);
         } else if (food_y == i && food_x == j) {
           putc('#', stdout);
         } else {
            putc('.', stdout);
         }
      }
      putc('\n', stdout);
   }
   ch_stack_node *tail2 = ch_stk_copy(tail);
   ch_stk_push(&__istack, ch_valof_stack(tail2));
   @return@
}

fn rest (tx : int ty : int tail : [int]) -> ([int]) {
→ ↻ ⧺ ⇈ 0 = ? 1 = ? ⊢! ↕ ⊢! ↻ rest ↻ H + H % ⤓ ↕ W + W % ⤓
            ↓     ↓
            ◌     → 2 drop ↻ H + H % ⤓ ↕ W + W % ⤓
}

fn ⊩ (x : [int]) -> (int [int]) {
→ ⊢! ↕ ⊢ ↷ ↕ ⤓ ↕
}

fn ⫣ (x : [int]) -> (int [int]) {
→ ⊣! ↕ ⊣ ↷ ⇆ ↕ ⤓ ⇆ ↕
}

fn snek (dx : int dy : int tail : [int]) -> ([int]) {
→ ↻ ⊢ ↻ + ↷ ⊩ ↻ + ↷ ↷ rest
}

fn ¿ded- (p : [int] tail : [int]) -> (bool) {
↓        '⊥               '⊤
          ↑                ↑
→ ↕ ⧺ 0 = ? 2 take ↻ ⇈ ↻ = ? ↓
                             ↕
    ↑                        ←
}

fn ¿ded (tail : [int]) -> (bool [int]) {
↓         →                  → ↻ ↕
         '⊥                 '⊤
          ↑                  ↑
→ ⇈ ⧺ 0 = ? 2 take ⊼ ↕ ¿ded- ? ↓
    ↑                          ←
}

fn ¿eat (tail : [int]) -> (bool int int [int]) cffi {
   ch_stack_node *call = @(W)@(NULL);
   int W = ch_stk_pop(&call).value.i;
   call = @(H)@(NULL);
   int H = ch_stk_pop(&call).value.i;

   static int food_x, food_y;
   static char is_init = 0;
   int head_x = tail->val.value.i;
   int head_y = tail->next->val.value.i;
   char is_eat = 0;
   if (!is_init) {
      printf("Here\n");
      food_x = rand() % W;
      food_y = rand() % H;
      is_init = 1;
   }
   if (head_x == food_x && head_y == food_y) {
      is_eat = 1;
      is_init = 0;
   }

   ch_stk_push(&__istack, ch_valof_stack(tail));
   tail = NULL;
   ch_stk_push(&__istack, ch_valof_int(food_y));
   ch_stk_push(&__istack, ch_valof_int(food_x));
   ch_stk_push(&__istack, ch_valof_bool(is_eat));
   @return@
}

fn seed () -> () cffi {
   srand(time(NULL));
   @return@
}

fn eat (tail : [int]) -> ([int] int int) {
→ ¿eat ¬ ? ↻ ⇆ 4 ↙ ⊩ ↕ ⊣ ↻ 2 * - ↕ ⊢ ↕ ⫣ ↻ 2 * - ↕ ↻ ⤓ ↕ ⤓ ↕ ++ ⇆
         ↓
         ↻
}

fn loop () -> () {
↓                   → "Dead" print
                    ↑
→ 1 0 1 1 1 2 ▭ ↓   ? ¿ded     ←       ←         ←
               eat
              draw
                †
                ⇈
                ⇑
                =
                ?→ ◌ -1 0 snek ↑      snek      snek
                ⇈                      -1        1
                ⇓                      0         0
                =                      ◌         ◌
                ?→ ◌ 1 0 snek  ↑       ↑         ↑
                →                ⇈ ⇐ = ?   ⇈ ⇒ = ?  "Quit" print
}

fn main () -> () {
→ seed unbuffer loop buffer
}
