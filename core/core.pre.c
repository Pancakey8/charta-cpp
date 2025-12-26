#ifdef PRE
#include "core.h"
#else
#include "core.pre.h"
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ch_string ch_str_new(char const *data) {
    ch_string str;
    size_t len = strlen(data);
    str.data = malloc(len + 1);
    strcpy(str.data, data);
    str.len = len;
    str.size = len + 1;
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
    case CH_VALK_STACK:
        return "stack";
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
    } else if (val->kind == CH_VALK_STACK) {
        ch_stk_delete(&val->value.stk);
    }
    val->kind = -1;
}

ch_value ch_valcpy(ch_value const *v);

ch_stack_node *ch_stk_copy(ch_stack_node *stk) {
    ch_stack_node *out = NULL;
    ch_stack_node **tail = &out;

    while (stk) {
        ch_stack_node *node = malloc(sizeof(ch_stack_node));
        node->val = ch_valcpy(&stk->val);
        node->next = NULL;

        *tail = node;
        tail = &node->next;

        stk = stk->next;
    }

    return out;
}

ch_value ch_valcpy(ch_value const *v) {
    ch_value other;
    other.kind = v->kind;
    if (v->kind == CH_VALK_STRING) {
        other.value.s = ch_str_new(v->value.s.data);
    } else if (v->kind == CH_VALK_STACK) {
        other.value.stk = ch_stk_copy(v->value.stk);
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

ch_stack_node *ch_stk_args(ch_stack_node **from, size_t n, char is_rest) {
    ch_stack_node *args = NULL;
    if (n != 0) {

        if (*from == NULL) {
            printf("ERR: Tried to pop '%zu' arguments, but stack is empty.\n",
                   n);
            exit(1);
        }

        args = *from;
        ch_stack_node *curr = *from;

        for (size_t i = 1; i < n; ++i) {
            curr = curr->next;
            if (curr == NULL) {
                printf("ERR: Tried to pop '%zu' arguments, but stack is too "
                       "short.\n",
                       n);
                exit(1);
            }
        }

        *from = curr->next;
        curr->next = NULL;
    }

    if (is_rest) {
        ch_stack_node *arg_ext = ch_stk_new();
        ch_stk_push(&arg_ext,
                    (ch_value){.kind = CH_VALK_STACK, .value.stk = *from});
        ch_stk_append(&arg_ext, args);
        *from = NULL;
        args = arg_ext;
    }

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
        printf("%d", v.value.i);
        break;
    case CH_VALK_FLOAT:
        printf("%f", v.value.f);
        break;
    case CH_VALK_BOOL:
        printf("%b", v.value.b);
        break;
    case CH_VALK_CHAR:
        printf("'\\U%x'", v.value.i);
        break;
    case CH_VALK_STRING:
        printf("%s", v.value.s.data);
        break;
    case CH_VALK_STACK: {
        ch_stack_node *n = v.value.stk;
        printf("[");
        while (n) {
            print_value(n->val);
            if (n->next) {
                printf(", ");
            }
            n = n->next;
        }
        printf("]");
    }
    }
}

void println_value(ch_value v) {
    print_value(v);
    printf("\n");
}

ch_stack_node *_mangle_(print, "print")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    ch_value v = ch_stk_pop(&local);
    println_value(v);
    ch_val_delete(&v);
    return NULL;
}

ch_stack_node *_mangle_(dup, "dup")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    ch_value v = ch_stk_pop(&local);
    ch_value cp = ch_valcpy(&v);
    ch_stk_push(&local, v);
    ch_stk_push(&local, cp);
    return local;
}

ch_stack_node *_mangle_(swp, "swp")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value a = ch_stk_pop(&local);
    ch_value b = ch_stk_pop(&local);
    ch_stk_push(&local, a);
    ch_stk_push(&local, b);
    return local;
}

ch_stack_node *_mangle_(dbg, "dbg")(ch_stack_node **full) {
    ch_stack_node *elem = *full;
    size_t i = 0;
    printf("DEBUG:\n");
    while (elem) {
        printf("%zu | ", i);
        println_value(elem->val);
        elem = elem->next;
    }
    return NULL;
}

ch_stack_node *_mangle_(equ_cmp, "=")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (a.kind != b.kind) {
        ch_stk_push(&local, ch_valof_bool(0));
        return local;
    }

    char res;

    switch (a.kind) {
    case CH_VALK_INT:
        res = a.value.i == b.value.i;
        break;
    case CH_VALK_FLOAT:
        res = a.value.f == b.value.f;
        break;
    case CH_VALK_BOOL:
        res = a.value.b == b.value.b;
        break;
    case CH_VALK_CHAR:
        res = a.value.i == b.value.i;
        break;
    case CH_VALK_STRING:
        res = strcmp(a.value.s.data, b.value.s.data) == 0;
        break;
    case CH_VALK_STACK:
        printf("'=' not implemented between stacks\n");
        exit(1);
    }

    ch_stk_push(&local, ch_valof_bool(res));
    ch_val_delete(&a);
    ch_val_delete(&b);
    return local;
}

ch_stack_node *_mangle_(sub, "-")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
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
    ch_stack_node *local = ch_stk_args(full, 2, 0);
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

ch_stack_node *_mangle_(boxstk, "box")(ch_stack_node **full) {
    ch_value val;
    val.kind = CH_VALK_STACK;
    val.value.stk = *full;
    ch_stack_node *stk = ch_stk_new();
    ch_stk_push(&stk, val);
    *full = NULL;
    return stk;
}

ch_stack_node *_mangle_(pop, "pop")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    ch_value val = ch_stk_pop(&local);
    ch_val_delete(&val);
    return local;
}

ch_stack_node *_mangle_(fst_pop, "fst!")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_STACK) {
        printf("ERR: '⊢!' expected stack, got '%s'\n",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    if (local->val.value.stk == NULL) {
        printf("ERR: '⊢!' got empty stack");
        exit(1);
    }
    ch_value val = ch_stk_pop(&local->val.value.stk);
    ch_stk_push(&local, val);
    return local;
}
