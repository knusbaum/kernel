#include "../stdio.h"
#include "../common.h"
#include "lexer.h"
#include "lstring.h"

struct lexer {
    FILE *f;
    int look;
};

struct lexer *new_lexer(FILE *f) {
    struct lexer *l = malloc(sizeof (struct lexer));
    l->f = f;
    l->look = 0;
    return l;
}

void destroy_lexer(struct lexer *lex) {
    //fclose(lex->f);
    free(lex);
}

int is_whitespace(int c) {
    return
        c == ' ' ||
        c == '\n' ||
        c == '\t';
}

int is_sym_char(int c) {
    return
        !is_whitespace(c) &&
        c != '(' &&
        c != ')';
}

static void skip_comments(struct lexer *lex) {
    if(lex->look == ';') {
        while(lex->look != '\n') {
            lex->look = getc(lex->f);
        }
    }
}

//int lex->look;
static int look(struct lexer *lex) {
    //printf("[lexer.c][look] Looking.\n");
    if(!lex->look) {
        lex->look = getc(lex->f);
        skip_comments(lex);
    }
    return lex->look;
}

static int get_char(struct lexer *lex) {
    //printf("[lexer.c][get_char] Getting char.\n");
    if(!lex->look) {
        lex->look = getc(lex->f);
        skip_comments(lex);
    }
    int ret = lex->look;
    lex->look = getc(lex->f);
    skip_comments(lex);
    //printf("[lexer.c][get_char] Got: %c\n", ret);
    return ret;
}

static void skip_whitespace(struct lexer *lex) {
    //printf("[lexer.c][next_token] Skipping whitespace.\n");
    while(is_whitespace(look(lex))) {
        get_char(lex);
    }
}

static void match(struct lexer *lex, int c) {
    //printf("[lexer.c][match] Matching %c.\n", c);
    if(look(lex) == c) {
        //get_char();
        lex->look = 0;
        return;
    }
    // This should never happen. If it does, it is a bug in the VM.
    printf("BAD MATCH! Wanted: %c, got: %c\n", c, look(lex));
    printf("This is a bug. Please report this.\n");
    PANIC("LISP QUIT!");
}

static void append_escaped(string *s, int c) {
    switch(c) {
    case 'n':
        string_append(s, '\n');
        break;
    case 't':
        string_append(s, '\t');
        break;
    default:
        break;
    }
}

string *parse_string(struct lexer *lex) {
    match(lex, '"');
    int c;
    string *s = new_string();
    while((c = look(lex)) != '"') {
        switch(c) {
        case '\\':
            match(lex, '\\');
            append_escaped(s, look(lex));
            get_char(lex);
            break;
        default:
            string_append(s, c);
            get_char(lex);
        }
    }
    match(lex, '"');
    return s;
}

char parse_character(struct lexer *lex) {
    match(lex, '\\');
    char c = look(lex);
    get_char(lex);
    return c;
}

string *parse_symbol(struct lexer *lex) {
    string *s = new_string();
    int c;
    while(c = look(lex), is_sym_char(c)) {
        if(c >=97 && c <= 122) {
            c -= 32;
        }
        string_append(s, c);
        get_char(lex);
    }
    return s;
}

long parse_long(struct lexer *lex) {
    string *s = new_string();
    while(isdigit(look(lex))) {
        string_append(s, look(lex));
        get_char(lex);
    }
    long ret = strtol(string_ptr(s), NULL, 0);
    string_free(s);
    return ret;
}

void next_token(struct lexer *lex, struct token *t) {
    skip_whitespace(lex);

    switch(look(lex)) {
    case '(':
        t->type = LPAREN;
        t->data = NULL;
        get_char(lex);
        break;
    case ')':
        t->type = RPAREN;
        t->data = NULL;
        get_char(lex);
        break;
    case '\'':
        t->type = QUOTE;
        t->data = NULL;
        get_char(lex);
        break;
    case '"':
        t->type = STRING;
        t->data = parse_string(lex);
        break;
    case '.':
        t->type = DOT;
        t->data = NULL;
        get_char(lex);
        break;
    case ':':
        t->type = KEYWORD;
        t->data = parse_symbol(lex);
        break;
    case '`':
        t->type = BACKTICK;
        t->data = NULL;
        get_char(lex);
        break;
    case ',':
        t->type = COMMA;
        t->data = NULL;
        get_char(lex);
        break;
    case '@':
        t->type = AT_SYMBOL;
        t->data = NULL;
        get_char(lex);
        break;
    case '#':
        t->type = CHARACTER;
        get_char(lex); // We can throw away the '#'
        t->character = parse_character(lex);
        break;
    case EOF:
        t->type = END;
        t->data = NULL;
        break;
    default:
        if(isdigit(look(lex))) {
            t->type = NUM;
            t->num = parse_long(lex);
//            get_char();
        }
        //if(isalpha(look())) {
        else {
            t->type = SYM;
            t->data = parse_symbol(lex);
        }
//        else {
//            //printf("[lexer.c][next_token] Got invalid character: %c\n", look());
//            abort();
//        }
    }
//    printf("[lexer.c][next_token] Made token @ %p: ", t);
//    print_token(t);
}

void free_token(struct token *t) {
    switch(t->type) {
    case SYM:
    case KEYWORD:
    case STRING:
        string_free(t->data);
        break;
    case LPAREN:
    case RPAREN:
    case QUOTE:
    case BACKTICK:
    case COMMA:
    case AT_SYMBOL:
    case END:
    case NUM:
    case NONE:
    case CHARACTER:
    case DOT:
        break;
    }
}

const char *toktype_str(enum toktype t) {
    switch(t) {
    case NONE:
        return "NONE";
        break;
    case LPAREN:
        return "LPAREN";
        break;
    case RPAREN:
        return "RPAREN";
        break;
    case SYM:
        return "SYM";
        break;
    case STRING:
        return "STRING";
        break;
    case NUM:
        return "NUM";
        break;
    case QUOTE:
        return "QUOTE";
        break;
    case BACKTICK:
        return "BACKTICK";
        break;
    case COMMA:
        return "COMMA";
        break;
    case AT_SYMBOL:
        return "AT_SYMBOL";
        break;
    case DOT:
        return "DOT";
        break;
    case CHARACTER:
        return "CHARACTER";
        break;
    case END:
        return "END";
        break;
    default:
        return "UNKNOWN";
    }
}

void print_token(struct token *t) {
    printf("[");
    switch(t->type) {
    case NONE:
    case LPAREN:
    case RPAREN:
    case QUOTE:
    case BACKTICK:
    case COMMA:
    case DOT:
    case CHARACTER:
    case END:
        printf("%s", toktype_str(t->type));
        break;
    case SYM:
        printf("%s, %s", toktype_str(t->type), string_ptr(t->data));
        break;
    case STRING:
        printf("%s, \"%s\"", toktype_str(t->type), string_ptr(t->data));
        break;
    case NUM:
        printf("%s, %ld", toktype_str(t->type), t->num);
        break;
    default:
        printf("UNKNOWN: %d", t->type);
        break;
    }
    printf("]\n");
}

