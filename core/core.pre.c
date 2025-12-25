#include "core.pre.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ch_string ch_str_new(char const *data) {
    ch_string str;
    size_t len = strlen(data + 1);
    str.data = malloc(len);
    strcpy(str.data, data);
    str.len = len;
    str.size = len;
    return str;
}

void ch_str_delete(ch_string *str) {
    free(str->data);
    str->data = NULL;
}

ch_value ch_valof_int(int n) {
    return (ch_value){.kind = CH_VALK_INT, .value.i = n};
}

ch_value ch_valof_float(float n) {
    return (ch_value){.kind = CH_VALK_FLOAT, .value.f = n};
}

// UTF32 codepoint
ch_value ch_valof_char(int n) {
    return (ch_value){.kind = CH_VALK_CHAR, .value.i = n};
}

ch_value ch_valof_string(ch_string n) {
    return (ch_value){.kind = CH_VALK_STRING, .value.s = n};
}

ch_value ch_valof_bool(char n) {
    return (ch_value){.kind = CH_VALK_BOOL, .value.b = n};
}

char *ch_valk_name(ch_value_kind k) {
    switch (k) {
    case CH_VALK_INT:
        return "int";
    case CH_VALK_FLOAT:
        return "float";
    case CH_VALK_BOOL:
        return "bool";
    case CH_VALK_CHAR:
        return "char";
    case CH_VALK_STRING:
        return "string";
    }
}

char ch_valas_bool(ch_value v) {
    if (v.kind != CH_VALK_BOOL) {
        printf("ERR: Expected 'bool', got '%s'\n", ch_valk_name(v.kind));
        exit(1);
    }
    return v.value.b;
}

void ch_val_delete(ch_value *val) {
    if (val->kind == CH_VALK_STRING) {
        ch_str_delete(&val->value.s);
    }
    val->kind = -1;
}

ch_value ch_valcpy(ch_value const *v) {
    ch_value other;
    other.kind = v->kind;
    if (v->kind == CH_VALK_STRING) {
        other.value.s = ch_str_new(v->value.s.data);
    } else {
        other.value = v->value;
    }
    return other;
}

ch_stack_node *ch_stk_new() { return NULL; }

void ch_stk_push(ch_stack_node **stk, ch_value val) {
    ch_stack_node *new = malloc(sizeof(ch_stack_node));
    new->val = val;
    new->next = *stk;
    *stk = new;
}

ch_value ch_stk_pop(ch_stack_node **stk) {
    ch_value v = (*stk)->val;
    ch_stack_node *next = (*stk)->next;
    free(*stk);
    *stk = next;
    return v;
}

ch_stack_node *ch_stk_args(ch_stack_node **from, size_t n) {
    if (n == 0 || *from == NULL)
        return NULL;

    ch_stack_node *args = *from;
    ch_stack_node *curr = *from;

    for (size_t i = 1; i < n; ++i) {
        curr = curr->next;
        if (curr == NULL) {
            printf(
                "ERR: Tried to pop '%zu' arguments, but stack is too short.\n",
                n);
            exit(1);
        }
    }

    *from = curr->next;
    curr->next = NULL;

    return args;
}

///! MOVES
void ch_stk_append(ch_stack_node **to, ch_stack_node *from) {
    if (from) {
        ch_stack_node *end = from;
        while (end->next != NULL) {
            end = end->next;
        }
        end->next = *to;
        *to = from;
    }
}

void ch_stk_delete(ch_stack_node **stk) {
    ch_stack_node *head = *stk;
    while (head) {
        ch_val_delete(&head->val);
        ch_stack_node *next = head->next;
        free(head);
        head = next;
    }
    *stk = NULL;
}

void print_value(ch_value v) {
    switch (v.kind) {
    case CH_VALK_INT:
        printf("%d\n", v.value.i);
        break;
    case CH_VALK_FLOAT:
        printf("%f\n", v.value.f);
        break;
    case CH_VALK_BOOL:
        printf("%b\n", v.value.b);
        break;
    case CH_VALK_CHAR:
        printf("'\\U%x'\n", v.value.i);
        break;
    case CH_VALK_STRING:
        printf("%s\n", v.value.s.data);
        break;
    }
}

ch_stack_node *_mangle_(print, "print")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1);
    ch_value v = ch_stk_pop(&local);
    print_value(v);
    return NULL;
}

ch_stack_node *_mangle_(dup, "dup")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1);
    ch_value v = ch_stk_pop(&local);
    ch_value cp = ch_valcpy(&v);
    ch_stk_push(&local, v);
    ch_stk_push(&local, cp);
    return local;
}

ch_stack_node *_mangle_(swp, "swp")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2);
    ch_value a = ch_stk_pop(&local);
    ch_value b = ch_stk_pop(&local);
    ch_stk_push(&local, a);
    ch_stk_push(&local, b);
    return local;
}

ch_stack_node *_mangle_(dbg, "dbg")(ch_stack_node **full) {
    ch_stack_node *elem = *full;
    size_t i = 0;
    while (elem) {
        printf("%zu | ", i);
        print_value(elem->val);
        elem = elem->next;
    }
    return NULL;
}

ch_stack_node *_mangle_(equ_cmp, "=")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (a.kind != b.kind) {
        ch_stk_push(&local, ch_valof_bool(0));
        return local;
    }

    switch (a.kind) {
    case CH_VALK_INT:
        ch_stk_push(&local, ch_valof_bool(a.value.i == b.value.i));
        return local;
    case CH_VALK_FLOAT:
        ch_stk_push(&local, ch_valof_bool(a.value.f == b.value.f));
        return local;
    case CH_VALK_BOOL:
        ch_stk_push(&local, ch_valof_bool(a.value.b == b.value.b));
        return local;
    case CH_VALK_CHAR:
        ch_stk_push(&local, ch_valof_bool(a.value.i == b.value.i));
        return local;
    case CH_VALK_STRING:
        ch_stk_push(&local,
                    ch_valof_bool(strcmp(a.value.s.data, b.value.s.data) == 0));
        return local;
    }
}

ch_stack_node *_mangle_(sub, "-")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (a.kind == CH_VALK_INT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_int(a.value.i - b.value.i));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_float(a.value.f - b.value.i));
    } else if (a.kind == CH_VALK_INT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(a.value.i - b.value.f));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(a.value.f - b.value.f));
    } else {
        printf("ERR: '-' expected two numbers, got '%s' and '%s'\n",
               ch_valk_name(a.kind), ch_valk_name(b.kind));
        exit(1);
    }
    return local;
}

ch_stack_node *_mangle_(add, "+")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (a.kind == CH_VALK_INT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_int(a.value.i + b.value.i));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_float(a.value.f + b.value.i));
    } else if (a.kind == CH_VALK_INT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(a.value.i + b.value.f));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(a.value.f + b.value.f));
    } else {
        printf("ERR: '+' expected two numbers, got '%s' and '%s'\n",
               ch_valk_name(a.kind), ch_valk_name(b.kind));
        exit(1);
    }
    return local;
}
