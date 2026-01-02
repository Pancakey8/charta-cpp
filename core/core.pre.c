#ifdef PRE
#include "core.h"
#else
#include "core.pre.h"
#endif
#include <math.h>
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

ch_string ch_str_alloc(size_t len) {
    ch_string str;
    str.size = len + 1;
    str.len = 0;
    str.data = malloc(len + 1);
    str.data[0] = 0;
    return str;
}

void ch_str_push(ch_string *str, char c) {
    if (str->len + 1 >= str->size) {
        str->size = 3 * str->size / 2 + 1;
        str->data = realloc(str->data, str->size);
    }
    str->data[str->len] = c;
    str->data[str->len + 1] = 0;
    ++str->len;
}

void ch_str_append(ch_string *str, ch_string *other) {
    if (other->len == 0) {
        return;
    }

    size_t needed = str->len + other->len + 1;

    if (needed > str->size) {
        size_t new_size = needed * 2;
        char *new_data = realloc(str->data, new_size);
        str->data = new_data;
        str->size = new_size;
    }

    memcpy(str->data + str->len, other->data, other->len);
    str->len += other->len;
    str->data[str->len] = '\0';
}

void ch_str_delete(ch_string *str) {
    free(str->data);
    str->data = NULL;
    str->len = 0;
    str->size = 0;
}

void ch_str_replace(ch_string *str, size_t pos, size_t n,
                    ch_string const *rep) {
    if (pos > str->len)
        pos = str->len;

    if (pos + n > str->len)
        n = str->len - pos;

    size_t new_len = str->len - n + rep->len;

    if (new_len > str->size) {
        str->size = new_len + 1;
        str->data = realloc(str->data, str->size);
    }

    if (rep->len != n) {
        memmove(str->data + pos + rep->len, str->data + pos + n,
                str->len - (pos + n));
    }

    memcpy(str->data + pos, rep->data, rep->len);

    str->len = new_len;
    str->data[str->len] = '\0';
}

ch_string encode_utf8(int c) {
    if (c <= 0x7F) {
        ch_string out = ch_str_alloc(1);
        ch_str_push(&out, c);
        return out;
    } else if (c <= 0x7FF) {
        ch_string out = ch_str_alloc(1);
        ch_str_push(&out, (char)(0xC0 | (c >> 6)));
        ch_str_push(&out, (char)(0x80 | (c & 0x3F)));
        return out;
    } else if (c <= 0xFFFF) {
        ch_string out = ch_str_alloc(1);
        ch_str_push(&out, (char)(0xE0 | (c >> 12)));
        ch_str_push(&out, (char)(0x80 | ((c >> 6) & 0x3F)));
        ch_str_push(&out, (char)(0x80 | (c & 0x3F)));
        return out;
    } else {
        ch_string out = ch_str_alloc(1);
        ch_str_push(&out, (char)(0xF0 | (c >> 18)));
        ch_str_push(&out, (char)(0x80 | ((c >> 12) & 0x3F)));
        ch_str_push(&out, (char)(0x80 | ((c >> 6) & 0x3F)));
        ch_str_push(&out, (char)(0x80 | (c & 0x3F)));
        return out;
    }
}

int decode_utf(ch_string const *str, size_t pos, size_t *bytes) {
    *bytes = 0;
    if (pos >= str->len)
        return 0;
    unsigned char c = str->data[pos];

    if (c < 0x80) {
        *bytes = 1;
        return c;
    } else if ((c >> 5) == 0b110 && pos + 1 < str->len) {
        *bytes = 2;
        return ((c & 31) << 6) | (str->data[pos + 1] & 63);
    } else if ((c >> 4) == 0b1110 && pos + 2 < str->len) {
        *bytes = 3;
        return ((c & 15) << 12) | ((str->data[pos + 1] & 63) << 6) |
               (str->data[pos + 2] & 63);
    } else if ((c >> 3) == 0b11110 && pos + 3 < str->len) {
        *bytes = 4;
        return ((c & 7) << 18) | ((str->data[pos + 1] & 63) << 12) |
               ((str->data[pos + 2] & 63) << 6) | (str->data[pos + 3] & 63);
    }

    *bytes = 0;
    return 0;
}

ssize_t utf8_nth_index(ch_string const *str, size_t n) {
    size_t pos = 0;
    size_t i = 0;
    size_t bytes;

    while (pos < str->len) {
        decode_utf(str, pos, &bytes);
        if (bytes == 0)
            return -1;

        if (i == n)
            return pos;

        pos += bytes;
        i++;
    }

    return -1;
}

ssize_t utf8_nth_char(ch_string const *str, size_t n) {
    size_t index = utf8_nth_index(str, n);
    if (index < 0)
        return -1;
    size_t bytes;
    int ch = decode_utf(str, index, &bytes);
    if (bytes == 0)
        return -1;
    return ch;
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

ch_value ch_valof_stack(struct ch_stack_node *n) {
    return (ch_value){.kind = CH_VALK_STACK, .value.stk = n};
}

ch_value ch_valof_opaque(void *ptr) {
    return (ch_value){.kind = CH_VALK_OPAQUE, .value.op = ptr};
}

ch_value
ch_valof_function(struct ch_stack_node *(*fn)(struct ch_stack_node **)) {
    return (ch_value){.kind = CH_VALK_FUNCTION, .value.fn = fn};
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
    case CH_VALK_OPAQUE:
        return "opaque";
    case CH_VALK_FUNCTION:
        return "function";
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
    } else if (v->kind == CH_VALK_OPAQUE) {
        printf("ERR: Tried to copy opaque value\n");
        exit(1);
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

ch_string ch_str_from_int(int v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", v);
    return ch_str_new(buf);
}

ch_string ch_str_from_float(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%f", v);
    return ch_str_new(buf);
}

ch_string print_value_str(ch_value v) {
    ch_string out = ch_str_new("");

    switch (v.kind) {

    case CH_VALK_INT: {
        ch_string s = ch_str_from_int(v.value.i);
        ch_str_append(&out, &s);
        ch_str_delete(&s);
        break;
    }

    case CH_VALK_FLOAT: {
        ch_string s = ch_str_from_float(v.value.f);
        ch_str_append(&out, &s);
        ch_str_delete(&s);
        break;
    }

    case CH_VALK_BOOL: {
        ch_string s = ch_str_new(v.value.b ? "⊤" : "⊥");
        ch_str_append(&out, &s);
        ch_str_delete(&s);
        break;
    }

    case CH_VALK_CHAR: {
        ch_string s = encode_utf8(v.value.i);
        ch_str_push(&out, '\'');
        ch_str_append(&out, &s);
        ch_str_push(&out, '\'');
        ch_str_delete(&s);
        break;
    }

    case CH_VALK_STRING:
        ch_str_append(&out, &v.value.s);
        break;

    case CH_VALK_STACK: {
        ch_stack_node *n = v.value.stk;

        ch_str_push(&out, '[');

        while (n) {
            ch_string elem = print_value_str(n->val);
            ch_str_append(&out, &elem);
            ch_str_delete(&elem);

            if (n->next) {
                ch_string comma = ch_str_new(", ");
                ch_str_append(&out, &comma);
                ch_str_delete(&comma);
            }

            n = n->next;
        }

        ch_str_push(&out, ']');
        break;
    }

    case CH_VALK_OPAQUE: {
        ch_string opaque = ch_str_new("<opaque>");
        ch_str_append(&out, &opaque);
        ch_str_delete(&opaque);
    }

    case CH_VALK_FUNCTION: {
        ch_string function = ch_str_new("<function>");
        ch_str_append(&out, &function);
        ch_str_delete(&function);
    }
    }

    return out;
}

void print_value(ch_value v) {
    ch_string s = print_value_str(v);
    printf("%s", s.data);
    ch_str_delete(&s);
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

ch_stack_node *_mangle_(over, "ovr")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value a = ch_stk_pop(&local);
    ch_value b = ch_stk_pop(&local);
    ch_value b2 = ch_valcpy(&b);
    ch_stk_push(&local, b);
    ch_stk_push(&local, a);
    ch_stk_push(&local, b2);
    return local;
}

ch_stack_node *_mangle_(pick, "pck")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 3, 0);
    ch_value val = ch_valcpy(&local->next->next->val);
    ch_stk_push(&local, val);
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

char val_equals(ch_value const *v1, ch_value const *v2) {
    if (v1->kind != v2->kind)
        return 0;

    switch (v1->kind) {
    case CH_VALK_INT:
        return v1->value.i == v2->value.i;
    case CH_VALK_FLOAT:
        return v1->value.f == v2->value.f;
    case CH_VALK_BOOL:
        return v1->value.b == v2->value.b;
    case CH_VALK_CHAR:
        return v1->value.i == v2->value.i;
    case CH_VALK_STRING:
        return strcmp(v1->value.s.data, v2->value.s.data) == 0;
    case CH_VALK_FUNCTION:
        return 0;
    case CH_VALK_OPAQUE:
        return 0;
    case CH_VALK_STACK: {
        ch_stack_node *s1 = v1->value.stk;
        ch_stack_node *s2 = v2->value.stk;
        while (s1 != NULL && s2 != NULL) {
            if (!val_equals(&s1->val, &s2->val))
                return 0;
            s1 = s1->next;
            s2 = s2->next;
        }
        return s1 == NULL && s2 == NULL;
    }
    }
}

ch_stack_node *_mangle_(equ_cmp, "=")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);

    ch_stk_push(&local, ch_valof_bool(val_equals(&a, &b)));
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

ch_stack_node *_mangle_(fst, "fst")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_STACK) {
        printf("ERR: '⊢' expected stack, got '%s'\n",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    if (local->val.value.stk == NULL) {
        printf("ERR: '⊢' got empty stack");
        exit(1);
    }
    ch_value top = ch_valcpy(&local->val.value.stk->val);
    ch_stk_push(&local, top);
    return local;
}

ch_stack_node *_mangle_(lst_pop, "lst!")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_STACK) {
        printf("ERR: '⊣!' expected stack, got '%s'\n",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    if (local->val.value.stk == NULL) {
        printf("ERR: '⊣!' got empty stack");
        exit(1);
    }
    ch_stack_node *stk = local->val.value.stk;
    ch_stack_node *prev = NULL;
    while (stk->next != NULL) {
        prev = stk;
        stk = stk->next;
    }
    if (prev) {
        prev->next = NULL;
    }
    ch_value val = ch_stk_pop(&stk);
    ch_stk_push(&local, val);
    return local;
}

ch_stack_node *_mangle_(lst, "lst")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_STACK) {
        printf("ERR: '⊣' expected stack, got '%s'\n",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    if (local->val.value.stk == NULL) {
        printf("ERR: '⊣' got empty stack");
        exit(1);
    }
    ch_stack_node *stk = local->val.value.stk;
    while (stk->next != NULL) {
        stk = stk->next;
    }
    ch_value val = ch_valcpy(&stk->val);
    ch_stk_push(&local, val);
    return local;
}

ch_stack_node *_mangle_(rot, "rot")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 3, 0);
    ch_stack_node *bot = local->next->next;
    local->next->next = NULL;
    bot->next = local;
    return bot;
}

ch_stack_node *_mangle_(nequ, "!=")(ch_stack_node **full) {
    ch_stack_node *local = _mangle_(equ_cmp, "=")(full);
    local->val.value.b = !local->val.value.b;
    return local;
}
ch_stack_node *_mangle_(less, "<")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    float b_val;
    if (b.kind == CH_VALK_INT) {
        b_val = b.value.i;
    } else if (b.kind == CH_VALK_FLOAT) {
        b_val = b.value.f;
    } else {
        printf("ERR: '<' expected number, got '%s'\n", ch_valk_name(b.kind));
        exit(1);
    }
    float a_val;
    if (a.kind == CH_VALK_INT) {
        a_val = a.value.i;
    } else if (a.kind == CH_VALK_FLOAT) {
        a_val = a.value.f;
    } else {
        printf("ERR: '<' expected number, got '%s'\n", ch_valk_name(a.kind));
        exit(1);
    }
    ch_stk_push(&local, ch_valof_bool(a_val < b_val));
    return local;
}
ch_stack_node *_mangle_(grt, ">")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    float b_val;
    if (b.kind == CH_VALK_INT) {
        b_val = b.value.i;
    } else if (b.kind == CH_VALK_FLOAT) {
        b_val = b.value.f;
    } else {
        printf("ERR: '>' expected number, got '%s'\n", ch_valk_name(b.kind));
        exit(1);
    }
    float a_val;
    if (a.kind == CH_VALK_INT) {
        a_val = a.value.i;
    } else if (a.kind == CH_VALK_FLOAT) {
        a_val = a.value.f;
    } else {
        printf("ERR: '>' expected number, got '%s'\n", ch_valk_name(a.kind));
        exit(1);
    }
    ch_stk_push(&local, ch_valof_bool(a_val > b_val));
    return local;
}
ch_stack_node *_mangle_(less_equ, "<=")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    float b_val;
    if (b.kind == CH_VALK_INT) {
        b_val = b.value.i;
    } else if (b.kind == CH_VALK_FLOAT) {
        b_val = b.value.f;
    } else {
        printf("ERR: '<=' expected number, got '%s'\n", ch_valk_name(b.kind));
        exit(1);
    }
    float a_val;
    if (a.kind == CH_VALK_INT) {
        a_val = a.value.i;
    } else if (a.kind == CH_VALK_FLOAT) {
        a_val = a.value.f;
    } else {
        printf("ERR: '<=' expected number, got '%s'\n", ch_valk_name(a.kind));
        exit(1);
    }
    ch_stk_push(&local, ch_valof_bool(a_val <= b_val));
    return local;
}
ch_stack_node *_mangle_(grt_equ, ">=")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    float b_val;
    if (b.kind == CH_VALK_INT) {
        b_val = b.value.i;
    } else if (b.kind == CH_VALK_FLOAT) {
        b_val = b.value.f;
    } else {
        printf("ERR: '>=' expected number, got '%s'\n", ch_valk_name(b.kind));
        exit(1);
    }
    float a_val;
    if (a.kind == CH_VALK_INT) {
        a_val = a.value.i;
    } else if (a.kind == CH_VALK_FLOAT) {
        a_val = a.value.f;
    } else {
        printf("ERR: '>=' expected number, got '%s'\n", ch_valk_name(a.kind));
        exit(1);
    }
    ch_stk_push(&local, ch_valof_bool(a_val >= b_val));
    return local;
}

ch_stack_node *_mangle_(mult, "*")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (a.kind == CH_VALK_INT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_int(a.value.i * b.value.i));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_float(a.value.f * b.value.i));
    } else if (a.kind == CH_VALK_INT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(a.value.i * b.value.f));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(a.value.f * b.value.f));
    } else {
        printf("ERR: '*' expected two numbers, got '%s' and '%s'\n",
               ch_valk_name(a.kind), ch_valk_name(b.kind));
        exit(1);
    }
    return local;
}
ch_stack_node *_mangle_(divd, "/")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (a.kind == CH_VALK_INT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_int(a.value.i / b.value.i));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_float(a.value.f / b.value.i));
    } else if (a.kind == CH_VALK_INT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(a.value.i / b.value.f));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(a.value.f / b.value.f));
    } else {
        printf("ERR: '/' expected two numbers, got '%s' and '%s'\n",
               ch_valk_name(a.kind), ch_valk_name(b.kind));
        exit(1);
    }
    return local;
}
ch_stack_node *_mangle_(mod, "%")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (a.kind == CH_VALK_INT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_int(a.value.i % b.value.i));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_INT) {
        ch_stk_push(&local, ch_valof_float(fmodf(a.value.f, b.value.i)));
    } else if (a.kind == CH_VALK_INT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(fmodf(a.value.f, b.value.i)));
    } else if (a.kind == CH_VALK_FLOAT && b.kind == CH_VALK_FLOAT) {
        ch_stk_push(&local, ch_valof_float(fmodf(a.value.f, b.value.i)));
    } else {
        printf("ERR: '%%' expected two numbers, got '%s' and '%s'\n",
               ch_valk_name(a.kind), ch_valk_name(b.kind));
        exit(1);
    }
    return local;
}

ch_stack_node *_mangle_(ins, "ins")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    if (local->next->val.kind != CH_VALK_STACK) {
        printf("ERR: 'ins' expected stack, got '%s'",
               ch_valk_name(local->next->val.kind));
        exit(1);
    }
    ch_value val = ch_stk_pop(&local);
    ch_stk_push(&local->val.value.stk, val);
    return local;
}

ch_stack_node *_mangle_(rot_rev, "rot-")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 3, 0);
    ch_stack_node *b = local->next;
    ch_stack_node *c = b->next;
    ch_stack_node *rest = c->next;
    c->next = local;
    local->next = rest;
    return b;
}

ch_stack_node *_mangle_(ord, "ord")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_CHAR) {
        printf("ERR: 'ord' expected char, got '%s'",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    local->val.kind = CH_VALK_INT;
    return local;
}

ch_stack_node *_mangle_(chr, "chr")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_INT) {
        printf("ERR: 'ord' expected int, got '%s'",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    local->val.kind = CH_VALK_CHAR;
    return local;
}

ch_stack_node *_mangle_(and, "&&")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (b.kind != CH_VALK_BOOL || a.kind != CH_VALK_BOOL) {
        printf("ERR: '&&' expected two bools, got '%s' and '%s'",
               ch_valk_name(b.kind), ch_valk_name(a.kind));
        exit(1);
    }
    ch_stk_push(&local, ch_valof_bool(b.value.b && a.value.b));
    return local;
}

ch_stack_node *_mangle_(or, "||")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (b.kind != CH_VALK_BOOL || a.kind != CH_VALK_BOOL) {
        printf("ERR: '||' expected two bools, got '%s' and '%s'",
               ch_valk_name(b.kind), ch_valk_name(a.kind));
        exit(1);
    }
    ch_stk_push(&local, ch_valof_bool(b.value.b || a.value.b));
    return local;
}

ch_stack_node *_mangle_(not, "!")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_BOOL) {
        printf("ERR: '!' expected bool, got '%s'",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    local->val.value.b = !local->val.value.b;
    return local;
}

ch_stack_node *_mangle_(type_int, "int")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 0, 0);
    ch_stk_push(&local, ch_valof_int(CH_VALK_INT));
    return local;
}
ch_stack_node *_mangle_(type_flt, "float")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 0, 0);
    ch_stk_push(&local, ch_valof_int(CH_VALK_FLOAT));
    return local;
}
ch_stack_node *_mangle_(type_chr, "char")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 0, 0);
    ch_stk_push(&local, ch_valof_int(CH_VALK_CHAR));
    return local;
}
ch_stack_node *_mangle_(type_bool, "bool")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 0, 0);
    ch_stk_push(&local, ch_valof_int(CH_VALK_BOOL));
    return local;
}
ch_stack_node *_mangle_(type_str, "string")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 0, 0);
    ch_stk_push(&local, ch_valof_int(CH_VALK_STRING));
    return local;
}
ch_stack_node *_mangle_(type_stk, "stack")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 0, 0);
    ch_stk_push(&local, ch_valof_int(CH_VALK_STACK));
    return local;
}
ch_stack_node *_mangle_(type_of, "type")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    ch_stk_push(&local, ch_valof_int(local->val.kind));
    return local;
}

ch_stack_node *_mangle_(dpt, "dpt")(ch_stack_node **full) {
    size_t len = 0;
    ch_stack_node *top = *full;
    while (top != NULL) {
        ++len;
        top = top->next;
    }
    ch_stack_node *local = ch_stk_new();
    ch_stk_push(&local, ch_valof_int(len));
    return local;
}

ch_stack_node *_mangle_(len, "len")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_STACK) {
        printf("ERR: 'len' expected stack, got '%s'\n",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    size_t len = 0;
    ch_stack_node *top = local->val.value.stk;
    while (top != NULL) {
        ++len;
        top = top->next;
    }
    ch_stk_push(&local, ch_valof_int(len));
    return local;
}

ch_stack_node *_mangle_(concat, "++")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value b = ch_stk_pop(&local);
    ch_value a = ch_stk_pop(&local);
    if (b.kind != CH_VALK_STACK || a.kind != CH_VALK_STACK) {
        printf("ERR: '++' expected two stacks, got '%s' and '%s'",
               ch_valk_name(b.kind), ch_valk_name(a.kind));
        exit(1);
    }
    ch_stk_append(&b.value.stk, a.value.stk);
    ch_stk_push(&local, b);
    return local;
}
void split_stack(ch_stack_node *stk, int n, ch_stack_node **taken,
                 ch_stack_node **rest) {
    if (n <= 0) {
        *taken = NULL;
        *rest = stk;
        return;
    }

    ch_stack_node *cur = stk;
    for (int i = 1; i < n; ++i) {
        if (!cur) {
            printf("ERR: expected %d elements but fell short", n);
            exit(1);
        }
        cur = cur->next;
    }

    if (!cur) {
        printf("ERR: expected %d elements but fell short", n);
        exit(1);
    }

    *taken = stk;
    *rest = cur->next;
    cur->next = NULL;
}
ch_stack_node *_mangle_(take, "take")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value n = ch_stk_pop(&local);
    ch_value s = ch_stk_pop(&local);

    if (n.kind != CH_VALK_INT || s.kind != CH_VALK_STACK) {
        printf("ERR: 'take' expected int then stack, got '%s' then '%s'",
               ch_valk_name(n.kind), ch_valk_name(s.kind));
        exit(1);
    }

    ch_stack_node *taken, *rest;
    split_stack(s.value.stk, n.value.i, &taken, &rest);

    ch_stk_push(&local, ch_valof_stack(rest));
    ch_stk_push(&local, ch_valof_stack(taken));
    return local;
}
ch_stack_node *_mangle_(drop, "drop")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value n = ch_stk_pop(&local);
    ch_value s = ch_stk_pop(&local);

    if (n.kind != CH_VALK_INT || s.kind != CH_VALK_STACK) {
        printf("ERR: 'drop' expected int then stack, got '%s' then '%s'",
               ch_valk_name(n.kind), ch_valk_name(s.kind));
        exit(1);
    }

    ch_stack_node *taken, *rest;
    split_stack(s.value.stk, n.value.i, &taken, &rest);

    while (taken) {
        ch_stack_node *next = taken->next;
        ch_val_delete(&taken->val);
        free(taken);
        taken = next;
    }

    ch_stk_push(&local, ch_valof_stack(rest));
    return local;
}
ch_stack_node *_mangle_(rev, "rev")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_STACK) {
        printf("ERR: 'rev' expected stack, got %s",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    ch_stack_node *prev = NULL;
    ch_stack_node *stk = local->val.value.stk;
    while (stk) {
        ch_stack_node *next = stk->next;
        stk->next = prev;
        prev = stk;
        stk = next;
    }
    local->val.value.stk = prev;
    return local;
}

ch_stack_node *_mangle_(str, "str")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    ch_value val = ch_stk_pop(&local);
    ch_string str = print_value_str(val);
    ch_val_delete(&val);
    ch_stk_push(&local, ch_valof_string(str));
    return local;
}

ch_stack_node *_mangle_(slen, "slen")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    if (local->val.kind != CH_VALK_STRING) {
        printf("ERR: 'slen' expected string, got %s",
               ch_valk_name(local->val.kind));
        exit(1);
    }
    ch_stk_push(&local, ch_valof_int(local->val.value.s.len));
    return local;
}

ch_stack_node *_mangle_(strget, "@")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value index = ch_stk_pop(&local);
    if (local->val.kind != CH_VALK_STRING || index.kind != CH_VALK_INT) {
        printf("ERR: '@' expected int and string, got %s and %s",
               ch_valk_name(index.kind), ch_valk_name(local->val.kind));
        exit(1);
    }
    size_t bytes;
    int code = utf8_nth_char(&local->val.value.s, index.value.i);
    if (bytes < 0) {
        printf("ERR: '@' failed to read char, possible out of bound access.");
        exit(1);
    }
    ch_stk_push(&local, ch_valof_char(code));
    return local;
}

ch_stack_node *_mangle_(strset, "@!")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 3, 0);
    ch_value index = ch_stk_pop(&local);
    ch_value ch = ch_stk_pop(&local);
    ch_string *string = &local->val.value.s;
    if (local->val.kind != CH_VALK_STRING || index.kind != CH_VALK_INT ||
        ch.kind != CH_VALK_CHAR) {
        printf("ERR: '@!' expected int,string,char; got %s,%s,%s",
               ch_valk_name(index.kind), ch_valk_name(local->val.kind),
               ch_valk_name(local->val.kind));
        exit(1);
    }
    ssize_t byte_idx = utf8_nth_index(string, index.value.i);
    if (byte_idx < 0) {
        printf("ERR: '@!' failed to read char, possible out of bound access.");
        exit(1);
    }
    ch_string s = encode_utf8(ch.value.i);
    ch_str_replace(string, byte_idx, 1, &s);
    ch_str_delete(&s);
    return local;
}

ch_stack_node *_mangle_(strapp, "&")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value s2 = ch_stk_pop(&local);
    ch_value s1 = ch_stk_pop(&local);
    if (s1.kind != CH_VALK_STRING || s2.kind != CH_VALK_STRING) {
        printf("ERR: '&' expected string and string, got %s and %s",
               ch_valk_name(s1.kind), ch_valk_name(s2.kind));
        exit(1);
    }
    ch_str_append(&s1.value.s, &s2.value.s);
    ch_val_delete(&s2);
    ch_stk_push(&local, s1);
    return local;
}

ch_stack_node *_mangle_(strpush, ".")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value c = ch_stk_pop(&local);
    ch_value s = ch_stk_pop(&local);
    if (c.kind != CH_VALK_CHAR || s.kind != CH_VALK_STRING) {
        printf("ERR: '&' expected char and string, got %s and %s",
               ch_valk_name(c.kind), ch_valk_name(s.kind));
        exit(1);
    }
    ch_string ap = encode_utf8(c.value.i);
    ch_str_append(&s.value.s, &ap);
    ch_str_delete(&ap);
    ch_stk_push(&local, s);
    return local;
}

ch_stack_node *_mangle_(fnapply, "ap")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 1, 0);
    ch_value val = ch_stk_pop(&local);
    if (val.kind != CH_VALK_FUNCTION) {
        printf("ERR: 'ap' expected function, got %s", ch_valk_name(val.kind));
        exit(1);
    }
    ch_stack_node *ret = val.value.fn(full);
    ch_stk_append(full, ret);
    return NULL;
}

ch_stack_node *_mangle_(fntail, "tail")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value val = ch_stk_pop(&local);
    if (val.kind != CH_VALK_FUNCTION) {
        printf("ERR: 'tail' expected function, got %s", ch_valk_name(val.kind));
        exit(1);
    }
    ch_stack_node *ret = val.value.fn(full);
    ch_stk_append(full, ret);
    return local;
}

ch_stack_node *_mangle_(repeat, "repeat")(ch_stack_node **full) {
    ch_stack_node *local = ch_stk_args(full, 2, 0);
    ch_value count = ch_stk_pop(&local);
    ch_value fn = ch_stk_pop(&local);
    if (count.kind != CH_VALK_INT || fn.kind != CH_VALK_FUNCTION) {
        printf("ERR: 'repeat' expected int and function, got %s and %s",
               ch_valk_name(count.kind), ch_valk_name(fn.kind));
        exit(1);
    }
    for (int i = 0; i < count.value.i; ++i) {
        ch_stack_node *ret = fn.value.fn(full);
        ch_stk_append(full, ret);
    }        
    return NULL;
}
