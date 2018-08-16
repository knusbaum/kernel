#ifndef COMPILER_H
#define COMPILER_H

#include "object.h"
#include "map.h"
#include "context.h"

struct binstr {
    void *instr;
    char has_arg;
    union {
        object *arg;
        long variance;
        char *str;
        size_t offset;
    };
};

#define CC_FLAG_HAS_REST 1

typedef struct compiled_chunk {
    struct binstr *bs;
    size_t b_off;
    size_t bsize;
    map_t *labels;
    char *name;
    long variance;
    unsigned char flags;
    long stacklevel;
    long saved_stacklevel;
    context *c;
} compiled_chunk;


void compiler_init();
compiled_chunk *compile_form(compiled_chunk *cc, context_stack *cs, object *o);
compiled_chunk *new_compiled_chunk();
void free_compiled_chunk(compiled_chunk *c);
void compile_fn(compiled_chunk *fn_cc, context_stack *cs, object *fn);
compiled_chunk *repl(context_stack *cs);
compiled_chunk *bootstrapper(context_stack *cs, FILE *bootstrap_file);

map_t *get_internals();

#endif
