#include "../stdio.h"
#include "../common.h"
#include "parser.h"
#include "lexer.h"
#include "map.h"
#include "context.h"
#include "threaded_vm.h"

struct parser {
    struct lexer *lex;
    struct token current;
};

parser *new_parser(char *fname) {
    FILE *f = fopen(fname, "r");
    return new_parser_file(f);
}

parser *new_parser_file(FILE *f) {
    parser *p = malloc(sizeof (parser));
    p->lex = new_lexer(f);
    p->current.type = NONE;
    return p;
}

void destroy_parser(parser *p) {
    destroy_lexer(p->lex);
    free(p);
}

void get_next_tok(parser *p) {
    //printf("[parser.c][get_next_tok] Getting next token.\n");
    if(p->current.type != NONE) {
        free_token(&p->current);
    }
    next_token(p->lex, &p->current);
    //printf("[parser.c][get_next_tok] Ended up with @ %p: ", &current);
    //print_token(&current);
}

void clear_tok(parser *p) {
    free_token(&p->current);
    p->current.type = NONE;
}

struct token *currtok(parser *p) {
    //printf("[parser.c][currtok] Getting currtok.\n");
    if(p->current.type == NONE) {
        //printf("[parser.c][currtok] Going to call next_token.\n");
        next_token(p->lex, &p->current);
    }
    //printf("[parser.c][currtok] currtok: ");
    //print_token(&current);
    return &p->current;
}

void tok_match(context_stack *cs, parser *p, enum toktype t) {
    //printf("[parser.c][tok_match] Matching toktype %s\n", toktype_str(t));
    if(currtok(p)->type != t) {
        printf("Failed to match toktype %s: got: ", toktype_str(t));
        print_token(currtok(p));
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    clear_tok(p);
}

object *parse_list(context_stack *cs, parser *p) {
    //printf("[parser.c][parse_list] Entering parse_list\n");
    object *car_obj = NULL;
    switch(currtok(p)->type) {
    case RPAREN:
        //printf("[parser.c][parse_list] Found end of list.\n");
        tok_match(cs, p, RPAREN);
        //return NULL;
        return obj_nil();
        break;
    default:
        //printf("[parser.c][parse_list] Getting next form for CAR.\n");
        car_obj = next_form(p, cs);
        break;
    }

    object *cdr_obj;
    switch(currtok(p)->type) {
    case DOT:
        tok_match(cs, p, DOT);
        cdr_obj = next_form(p, cs);
        tok_match(cs, p, RPAREN);
        break;
    default:
        cdr_obj = parse_list(cs, p);
        break;
    }

    object *conso = new_object_cons(car_obj, cdr_obj);
    return conso;
}

object *next_form(parser *p, context_stack *cs) {
    if(currtok(p)->type == NONE) get_next_tok(p);

    object *o;
    switch(currtok(p)->type) {
    case LPAREN:
        tok_match(cs, p, LPAREN);
        return parse_list(cs, p);
        break;
    case SYM:
        o = intern(new_string_copy(string_ptr(currtok(p)->data)));
        clear_tok(p);
        return o;
        break;
    case STRING:
        o = new_object(O_STR, new_string_copy(string_ptr(currtok(p)->data)));
        clear_tok(p);
        return o;
        break;
    case NUM:
        o = new_object_long(currtok(p)->num);
        clear_tok(p);
        return o;
        break;
    case CHARACTER:
        o = new_object_char(currtok(p)->character);
        clear_tok(p);
        return o;
        break;
    case KEYWORD:
        o = intern(new_string_copy(string_ptr(currtok(p)->data)));
        clear_tok(p);
        return o;
        break;
    case QUOTE:
        get_next_tok(p);
        o = next_form(p, cs);
        o = new_object_cons(o, obj_nil());
        return new_object_cons(intern(new_string_copy("QUOTE")), o);
        break;
    case BACKTICK:
        get_next_tok(p);
        o = next_form(p, cs);
        o = new_object_cons(o, obj_nil());
        return new_object_cons(intern(new_string_copy("BACKTICK")), o);
        break;
    case COMMA:
        get_next_tok(p);
        if(currtok(p)->type == AT_SYMBOL) {
            get_next_tok(p);
            o = next_form(p, cs);
            o = new_object_cons(o, obj_nil());
            return new_object_cons(intern(new_string_copy("COMMA_AT")), o);
        }
        else {
            o = next_form(p, cs);
            o = new_object_cons(o, obj_nil());
            return new_object_cons(intern(new_string_copy("COMMA")), o);
        }
        break;
    case AT_SYMBOL:
        //printf("[parser.c][next_form] @ symbol not with comma.\n");
        //abort();
        printf("Found '@' without a comma.\n");
        vm_error_impl(cs, interns("SIG-ERROR"));
        break;
    case END:
        vm_error_impl(cs, interns("END-OF-FILE"));
        get_next_tok(p);
        break;
    default:
        //printf("[parser.c][next_form] Got another token: ");
        //print_token(currtok(p));
        get_next_tok(p);
        return NULL;
    }
    return NULL;
}
