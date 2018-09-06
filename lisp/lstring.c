#include "../common.h"
#include "lstring.h"

/** String Machinery **/

#define STR_INITIAL_CAPACITY 8

struct string {
    char *s;
    size_t len;
    size_t cap;
};

string *new_string() {
    struct string *s = malloc(sizeof (struct string));
    s->s = NULL;
    s->cap = 0;
    s->len = 0;
    return s;
}

string *new_string_copy(const char *c) {
    struct string *s = malloc(sizeof (struct string));
    size_t size = strlen(c) + 1;
    s->s = malloc(size);
    strcpy(s->s, c);
    s->cap = size;
    s->len = size;
    return s;
}

void string_append(string *s, char c) {
    if(!s->s) {
        s->s = malloc(STR_INITIAL_CAPACITY);
        s->cap = STR_INITIAL_CAPACITY;
        s->len = 0;
    }

    if(s->len == s->cap - 1) {
        // time to expand.
        s->cap *= 2;
        s->s = realloc(s->s, s->cap);
    }

    s->s[s->len++] = c;
    s->s[s->len] = 0;
}

void string_trim_capacity(string *s) {
    s->s = realloc(s->s, s->len + 1);
    s->cap = s->len + 1;
}

size_t string_len(string *s) {
    return s->len;
}

size_t string_cap(string *s) {
    return s->cap;
}

const char *string_ptr(string *s) {
    if(s) {
        return s->s;
    }
    else {
        return "???";
    }
}

int string_cmp(string *s1, string *s2) {
    return strcmp(string_ptr(s1), string_ptr(s2));
}

int string_equal(string *s1, string *s2) {
    return string_cmp(s1, s2) == 0;
}

void string_free(string *s) {
    //memset(s->s, 0, s->cap);
    free(s->s);
    //memset(s, 0, sizeof (string));
    free(s);
}

