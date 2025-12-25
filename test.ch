fn printer (x : int) -> (int) {
-> dup 0 = ?  dup print 1 - printer
           |v
}

fn main () -> () {
-> 10 printer
}
