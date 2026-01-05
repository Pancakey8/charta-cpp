type user (name : string age : int admin : bool)

fn login (u : user) -> () {
→ user.name "(" & ↕ user.age str ↨ & ↕ user.admin ? ↕ ") has failed to log in"  & print
                                                  ↓
                                                  ↕
                                         ") has successfully logged in"
                                                  &
                                                  print
}

fn hog-memory () -> () {
→ ≍ 10000 ⋄
  ↓
  '⊤
  200
  "Big boy"
  user!
}

fn main () -> () {
→ '⊥ 4 "Toddler" user! login '⊤ 17 "Pancake" user! login hog-memory "Passed if no memory leak" print
}
