#ifndef LSTRING_H
#define LSTRING_H

#include "../stddef.h"

// String mechanics

typedef struct string string;

string *new_string();
string *new_string_copy(const char *c);
void string_append(string *s, char c);
void string_trim_capacity(string *s);
size_t string_len(string *s);
size_t string_cap(string *s);
const char *string_ptr(string *s);
int string_cmp(string *s1, string *s2);
int string_equal(string *s1, string *s2);
void string_free(string *s);

#endif
