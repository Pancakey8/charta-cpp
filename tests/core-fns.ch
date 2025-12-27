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
→ 1 2 3 4 5 ⇈ 5 ≠ ? 5 ≠ ? ↻ 2 ≠ ? ↷ 3 ≠ ? ↕ ⇈ 4 ≠ ? ◌ 1 ≠ ? 0
                  ↓     ↓       ↓       ↓         ↓       ↓
                  1     2       3       4         5       6
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

fn main () -> () {
→   "< > ≤ ≥" test-comparisons test ↓
↓            test test-equals "= ≠" ←
→   "⇈ ↕ ↻ ↷ ◌" test-align-ops test ↓
  test test-stk-ops "⊢ ⊢! ⊣ ⊣! ⤓ ▭" ←
}