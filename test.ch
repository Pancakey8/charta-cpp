fn test-arithmetic () -> () {
→ 2 3 + 4 * print 7 2 - print 5 8.0 / print 5 8 / print ↓
                            print % 2 3.5 print % 6 16  ←
}

fn test-comparisons () -> () {
→ 2 3 = print 3 5 ≠ print 2 3 < print 5 7 ≥ print 8 7 ≥ print
}

fn main () -> () {
→ test-arithmetic test-comparisons
}