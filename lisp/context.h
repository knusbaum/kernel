#ifndef CONTEXT_H
#define CONTEXT_H

#include "lexer.h"
#include "parser.h"
#include "object.h"
#include "map.h"

typedef struct context_stack context_stack;
typedef struct context context;
typedef struct context_var_iterator context_var_iterator;
typedef struct context_fn_iterator context_fn_iterator;

struct sym_val_pair {
    object *sym;
    object *val;
};

int sym_equal(void *a, void *b);

context_stack *context_stack_init();
void *destroy_context_stack(context_stack *cs);

//context *new_context();
context *top_context(context_stack *cs);
context *push_context(context_stack *cs);
context *push_existing_context(context_stack *cs, context *existing);
context *pop_context(context_stack *cs);
size_t context_level(context_stack *cs);
void pop_context_to_level(context_stack *cs, size_t level);
void free_context(context *c);
object *lookup_var(context_stack *cs, object *sym);
object *bind_var(context_stack *cs, object *sym, object *var);
object *lookup_fn(context_stack *cs, object *sym);
void bind_native_fn(context_stack *cs, object *sym, void (*fn)(context_stack *, long));
void bind_fn(context_stack *cs, object *sym, object *fn);
void unbind_fn(context_stack *cs, object *sym);

context_var_iterator *iterate_vars(context_stack *cs);
context_var_iterator *context_var_iterator_next(context_var_iterator *cvi);
struct sym_val_pair context_var_iterator_values(context_var_iterator *cvi);
void destroy_context_var_iterator(context_var_iterator *cvi);

context_fn_iterator *iterate_fns(context_stack *cs);
context_fn_iterator *context_fn_iterator_next(context_fn_iterator *cfi);
struct sym_val_pair context_fn_iterator_values(context_fn_iterator *cfi);
void destroy_context_fn_iterator(context_fn_iterator *cfi);

map_t *context_vars(context *c);
map_t *context_funcs(context *c);

#endif
