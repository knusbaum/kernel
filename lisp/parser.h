#ifndef PARSER_H
#define PARSER_H

#include "../stdio.h"
#include "lexer.h"
#include "object.h"
#include "context.h"

typedef struct parser parser;

/** Parser Operations **/
//parser *new_parser(char *fname);
parser *new_parser_file(FILE *f);
void destroy_parser(parser *p);
object *next_form(parser *p, context_stack *cs);

#endif
