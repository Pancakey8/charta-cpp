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
ch_stack_node *_mangle_(dbg, "dbg")(ch_stack_node **full);
ch_stack_node *_mangle_(equ_cmp, "=")(ch_stack_node **full);
ch_stack_node *_mangle_(add, "+")(ch_stack_node **full);
ch_stack_node *_mangle_(sub, "-")(ch_stack_node **full);
ch_stack_node *_mangle_(boxstk, "box")(ch_stack_node **full);
static inline ch_stack_node *_mangle_(boxstk2, "□")(ch_stack_node **full) {
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
