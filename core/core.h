#pragma once
#include <stddef.h>

typedef enum {
    CH_VALK_INT,
    CH_VALK_FLOAT,
    CH_VALK_BOOL,
    CH_VALK_CHAR,
    CH_VALK_STRING
} ch_value_kind;

typedef struct {
    char *data;
    size_t len;
    size_t size;
} ch_string;

ch_string ch_str_new(char const *data);

void ch_str_delete(ch_string *str);

typedef struct {
    ch_value_kind kind;
    union {
        int i;
        float f;
        char b;
        ch_string s;
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

ch_stack_node *ch_stk_args(ch_stack_node **from, size_t n);

void ch_stk_append(ch_stack_node **to, ch_stack_node *from);

void ch_stk_delete(ch_stack_node **stk);

ch_stack_node *__sprint(ch_stack_node **full);
ch_stack_node *__sdup(ch_stack_node **full);
/// =
ch_stack_node *__s__u61(ch_stack_node **full);
/// +
ch_stack_node *__s__u43(ch_stack_node **full);
/// -
ch_stack_node *__s__u45(ch_stack_node **full);
