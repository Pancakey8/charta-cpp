fn test (result : int name : string) -> () {
→ ⇈ 0 = ? "FAIL" print ↕ print print "" print
        ↓
        "OK"
        print
        ◌
        print
        ""
        print
}

fn example-stk () -> (... int) {
→ 1 2 3 4 5
}

fn test-equals () -> (int) {
→ 8 8 ≠ ? 13.5 13.5 = ? 2
        ↓             ↓
        1             → 'ü' 'ü' ≠ ? "hey" "hey" = ? 4 
                                  ↓               ↓
                                  3               → example-stk example-stk = ? 5
                                                                              ↓
                                                                              0
}

fn test-comparisons () -> (int) {
→ 3 5 > ? 5 5 < ? 6 8 >= ? 7 7 <= ? 4
        ↓       ↓        ↓        ↓
        1       2        3        → 7 8 <= ? 5
                                           ↓
                                           0
}

fn test-align-ops () -> (int) {
→ 1 2 3 4 5 ⇈ 5 ≠ ? 5 ≠ ? ↻ 2 ≠ ? ↷ 3 ≠ ? ↕ ⇈ 4 ≠ ? ◌ 1 ≠ ? 1 2 3 4 ≡ 4 ≠ ? ⊼ 3 ≠ ? 0
                  ↓     ↓       ↓       ↓         ↓       ↓               ↓       ↓
                  1     2       3       4         5       6               7       8
}

fn expected-stk () -> (... int) {
→ 2 3 4 7
}

fn test-stk-ops () -> (int) {
→ example-stk ⊢! 5 = ? 1
                     ↓
          2 ? = 1 ⊣! ←
            ↓
            → ⊢ 4 ≠ ? ⊣ 2 ≠ ? 7 ⤓ expected-stk = ? 5
                    ↓       ↓                    ↓
                    3       4                    1
                            ←                    2
                                                 3
                         6 ? = example-stk ▭ 5 4 ←
                           ↓
                           0
}

fn rev-example () -> (... int) {
→ 5 4 3 2 1
}

fn concat-example () -> (... int) {
→ 2 3 4 7 1 2 3 4 5
}

fn test-more-stk-ops () -> (int) {
↓                                                    0
                                                     ↑
                                                   4 ? = ↻ ↙ 3 ↘ 1 rev-example ◌ ↕ ↙ ←
                                                                                     3
                                                                                     ⇆
                                                                                    expected-stk
                                                                                     ↑ 
→ example-stk ⧺ 5 ≠ ? ⇆ rev-example ≠ ? example-stk expected-stk ++ concat-example = ? 3
                    ↓                 ↓                                             
                    1                 2
}

fn test-char-ops () -> (int) {
→ 'θ' ord ⇈ 952 ≠ ? chr 'θ' ≠ ? 0
                  ↓           ↓
                  1           2
}

fn test-bool-ops () -> (int) {
→ '⊤ '⊥ ∨ ? 1
          ↓
          '⊤
          '⊤      3
          ∧       ↑
          ?→ '⊤ ¬ ? 0
          2       
}

fn test-type-info () -> (int) {
↓                                                            4
                         0 ? ≠ bool ∈ '⊤ ◌ ? ≠ char ∈ 'a' ◌ ←?
                           ↓               ↓                 =
                           6               5                 int
                                                             ∈
                                                             3
                                                             ◌
→ "hello" ∈ string ≠ ? ∈ float = ? ◌ example-stk ∈ stack ≠ ? ↑
                     ↓           ↓                         ↓
                     1           2                         3
}

fn test-str-ops () -> (int) {
→ 13 str "13" ≠ ? "Süper" 2 @ 'p' ≠ ? "Hello" '←' 3 @! "Hel←o" ≠ ? "Hey" ℓ 3 = ? 4
                ↓                   ↓                            ↓             ↓
                1                   2                            3            "Yo"
                                    ←                            ←            '◡'
                                                                               .
                                                                              "Yo◡"
                                                                               =
                                       0 ? ≠ "Hello world" & "world" "Hello " ←?
                                         ↓                                     5
                                         6
}

fn test-apply () -> (int) {
↓                          2              3                4
                           ↑              ↑                ↑
→ 1 ≍ ▷ 2 ≠ ? ≍ 5 ⊼ ▷ 10 ≠ ? 'A' ⊼ ▷ 65 ≠ ? "hey" ↕ ▷ -5 ≠ ? 0
    ↓       ↓ ↓
    1       1 → ∈ char = ? ∈ int = ? -5
    +                    ↓         ↓
                        ord        2
                                   *
}

fn main () -> () {
→     "< > ≤ ≥" test-comparisons test ↓
↓              test test-equals "= ≠" ←
→ "≡ ⇈ ↕ ↻ ↷ ◌ ⊼" test-align-ops test ↓
↓   test test-stk-ops "⊢ ⊢! ⊣ ⊣! ⤓ ▭" ←
→        "ord chr" test-char-ops test ↓
↓          test test-bool-ops "∧ ∨ ¬" ←
→          "type" test-type-info test ↓
↓ test test-more-stk-ops "⧺ ⇆ ↙ ↘ ++" ←
→ "str ℓ @ @! . &" test-str-ops test  ↓
                  test test-apply "▷" ←
}