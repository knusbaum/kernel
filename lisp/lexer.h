#ifndef LEXER_H
#define LEXER_H

#include "../stdio.h"

enum toktype {
    NONE = 0,
    LPAREN = 1,
    RPAREN,
    SYM,
    KEYWORD,
    STRING,
    NUM,
    QUOTE,
    BACKTICK,
    COMMA,
    AT_SYMBOL,
    DOT,
    CHARACTER,
    END
};

struct token {
    enum toktype type;
    union {
        void *data;
        long num;
        char character;
    };
};

struct lexer;

struct lexer *new_lexer(FILE *f);
void destroy_lexer(struct lexer *lex);

void next_token(struct lexer *lex, struct token *t);
void free_token(struct token *t);
const char *toktype_str(enum toktype t);
void print_token(struct token *t);

#endif
