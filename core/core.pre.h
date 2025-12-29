#pragma once
#include <stddef.h>

#define _mangle_(x, a) x

typedef enum {
    CH_VALK_INT,
    CH_VALK_FLOAT,
    CH_VALK_BOOL,
    CH_VALK_CHAR,
    CH_VALK_STRING,
    CH_VALK_STACK
} ch_value_kind;

typedef struct {
    char *data;
    size_t len;
    size_t size;
} ch_string;

ch_string ch_str_new(char const *data);

ch_string ch_str_alloc(size_t len);

void ch_str_push(ch_string *str, char c);

void ch_str_append(ch_string *str, ch_string *other);
void ch_str_replace(ch_string *str, size_t pos, size_t n, ch_string const *rep);

void ch_str_delete(ch_string *str);

struct ch_stack_node;

typedef struct {
    ch_value_kind kind;
    union {
        int i;
        float f;
        char b;
        ch_string s;
        struct ch_stack_node *stk;
    } value;
} ch_value;

ch_value ch_valof_int(int n);

ch_value ch_valof_float(float n);

// UTF32 codepoint
ch_value ch_valof_char(int n);

ch_value ch_valof_string(ch_string n);

ch_value ch_valof_bool(char n);

ch_value ch_valof_stack(struct ch_stack_node *n);

char ch_valas_bool(ch_value v);

void ch_val_delete(ch_value *val);

typedef struct ch_stack_node {
    ch_value val;
    struct ch_stack_node *next;
} ch_stack_node;

ch_stack_node *ch_stk_new();

void ch_stk_push(ch_stack_node **stk, ch_value val);

ch_value ch_stk_pop(ch_stack_node **stk);

ch_stack_node *ch_stk_copy(ch_stack_node *stk);

ch_stack_node *ch_stk_args(ch_stack_node **from, size_t n, char is_rest);

void ch_stk_append(ch_stack_node **to, ch_stack_node *from);

void ch_stk_delete(ch_stack_node **stk);

ch_stack_node *_mangle_(print, "print")(ch_stack_node **full);

ch_stack_node *_mangle_(dup, "dup")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(dup2, "⇈")(ch_stack_node **full) {
    return _mangle_(dup, "dup")(full);
}
ch_stack_node *_mangle_(swp, "swp")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(swp2, "↕")(ch_stack_node **full) {
    return _mangle_(swp, "swp")(full);
}
ch_stack_node *_mangle_(over, "ovr")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(over2, "⊼")(ch_stack_node **full) {
    return _mangle_(over, "ovr")(full);
}
ch_stack_node *_mangle_(rot, "rot")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(rot2, "↻")(ch_stack_node **full) {
    return _mangle_(rot, "rot")(full);
}
ch_stack_node *_mangle_(rot_rev, "rot-")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(rot_rev2, "↷")(ch_stack_node **full) {
    return _mangle_(rot_rev, "rot-")(full);
}

ch_stack_node *_mangle_(dbg, "dbg")(ch_stack_node **full);

ch_stack_node *_mangle_(equ_cmp, "=")(ch_stack_node **full);
ch_stack_node *_mangle_(nequ, "!=")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(nequ2, "≠")(ch_stack_node **full) {
    return _mangle_(nequ, "!=")(full);
}
ch_stack_node *_mangle_(less, "<")(ch_stack_node **full);
ch_stack_node *_mangle_(grt, ">")(ch_stack_node **full);
ch_stack_node *_mangle_(less_equ, "<=")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(less_equ2, "≤")(ch_stack_node **full) {
    return _mangle_(less_equ, "<=")(full);
}
ch_stack_node *_mangle_(grt_equ, ">=")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(grt_equ2, "≥")(ch_stack_node **full) {
    return _mangle_(grt_equ, ">=")(full);
}

ch_stack_node *_mangle_(add, "+")(ch_stack_node **full);
ch_stack_node *_mangle_(sub, "-")(ch_stack_node **full);
ch_stack_node *_mangle_(mult, "*")(ch_stack_node **full);
ch_stack_node *_mangle_(divd, "/")(ch_stack_node **full);
ch_stack_node *_mangle_(mod, "%")(ch_stack_node **full);

ch_stack_node *_mangle_(boxstk, "box")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(boxstk2, "▭")(ch_stack_node **full) {
    return _mangle_(boxstk, "box")(full);
}

ch_stack_node *_mangle_(pop, "pop")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(pop2, "◌")(ch_stack_node **full) {
    return _mangle_(pop, "pop")(full);
}

ch_stack_node *_mangle_(fst_pop, "fst!")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(fst_pop2, "⊢!")(ch_stack_node **full) {
    return _mangle_(fst_pop, "fst!")(full);
}
ch_stack_node *_mangle_(fst, "fst")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(fst2, "⊢")(ch_stack_node **full) {
    return _mangle_(fst, "fst")(full);
}

ch_stack_node *_mangle_(lst_pop, "lst!")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(lst_pop2, "⊣!")(ch_stack_node **full) {
    return _mangle_(lst_pop, "lst!")(full);
}
ch_stack_node *_mangle_(lst, "lst")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(lst2, "⊣")(ch_stack_node **full) {
    return _mangle_(lst, "lst")(full);
}
ch_stack_node *_mangle_(ins, "ins")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(ins2, "⤓")(ch_stack_node **full) {
    return _mangle_(ins, "ins")(full);
}

ch_stack_node *_mangle_(ord, "ord")(ch_stack_node **full);
ch_stack_node *_mangle_(chr, "chr")(ch_stack_node **full);

ch_stack_node *_mangle_(and, "&&")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(and2, "∧")(ch_stack_node **full) {
    return _mangle_(and, "&&")(full);
}
ch_stack_node *_mangle_(or, "||")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(or2, "∨")(ch_stack_node **full) {
    return _mangle_(or, "||")(full);
}
ch_stack_node *_mangle_(not, "!")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(not2, "¬")(ch_stack_node **full) {
    return _mangle_(not, "!")(full);
}

ch_stack_node *_mangle_(type_int, "int")(ch_stack_node **full);
ch_stack_node *_mangle_(type_flt, "float")(ch_stack_node **full);
ch_stack_node *_mangle_(type_chr, "char")(ch_stack_node **full);
ch_stack_node *_mangle_(type_bool, "bool")(ch_stack_node **full);
ch_stack_node *_mangle_(type_str, "string")(ch_stack_node **full);
ch_stack_node *_mangle_(type_stk, "stack")(ch_stack_node **full);
ch_stack_node *_mangle_(type_of, "type")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(type_of2, "∈")(ch_stack_node **full) {
    return _mangle_(type_of, "type")(full);
}

ch_stack_node *_mangle_(dpt, "dpt")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(dpt2, "≡")(ch_stack_node **full) {
    return _mangle_(dpt, "dpt")(full);
}
ch_stack_node *_mangle_(len, "len")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(len2, "⧺")(ch_stack_node **full) {
    return _mangle_(len, "len")(full);
}

ch_stack_node *_mangle_(concat, "++")(ch_stack_node **full);
ch_stack_node *_mangle_(take, "take")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(take2, "↙")(ch_stack_node **full) {
    return _mangle_(take, "take")(full);
}
ch_stack_node *_mangle_(drop, "drop")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(drop2, "↘")(ch_stack_node **full) {
    return _mangle_(drop, "drop")(full);
}
ch_stack_node *_mangle_(rev, "rev")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(rev2, "⇆")(ch_stack_node **full) {
    return _mangle_(rev, "rev")(full);
}

ch_stack_node *_mangle_(str, "str")(ch_stack_node **full);
ch_stack_node *_mangle_(slen, "slen")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(slen2, "ℓ")(ch_stack_node **full) {
    return _mangle_(slen, "slen")(full);
}
ch_stack_node *_mangle_(strget, "@")(ch_stack_node **full);
ch_stack_node *_mangle_(strset, "@!")(ch_stack_node **full);
ch_stack_node *_mangle_(strapp, "&")(ch_stack_node **full);
ch_stack_node *_mangle_(strpush, ".")(ch_stack_node **full);
// panic
