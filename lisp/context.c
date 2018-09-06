#include "../stdio.h"
#include "../common.h"
#include "context.h"
#include "map.h"

struct context {
    map_t *vars;
    map_t *funcs;
};

#define CSTACK_INITIAL_SIZE 8
struct context_stack {
    struct context **cstack;
    size_t cstackoff;
    size_t cstacksize;
};

context *new_context();

context_stack *context_stack_init() {
    context_stack *cs = malloc(sizeof (struct context_stack));
    cs->cstack = malloc(sizeof (context *) * CSTACK_INITIAL_SIZE);
    cs->cstacksize = CSTACK_INITIAL_SIZE;
    cs->cstackoff = 0;
    cs->cstack[0] = new_context();
    return cs;
}

object *lookup_var_off(context_stack *cs, object *sym, size_t off) {
    object *o = map_get(cs->cstack[off]->vars, sym);
    if(!o && off > 0) {
        return lookup_var_off(cs, sym, off - 1);
    }
    return o;
}

object *lookup_var(context_stack *cs, object *sym) {
    return lookup_var_off(cs, sym, cs->cstackoff);
}

object *bind_var(context_stack *cs, object *sym, object *var) {
    //printf("Binding var: (");
    //print_object(sym);
    //printf(") -> (");
    //print_object(var);
    //printf(") context: %p\n", cs->cstack[cs->cstackoff]);
    map_put(cs->cstack[cs->cstackoff]->vars, sym, var);
    return var;
}

object *lookup_fn(context_stack *cs, object *sym) {
    // Get from slot 0. Function bindings should always be *global*
    object *o = map_get(cs->cstack[0]->funcs, sym);
    return o;
}

void bind_native_fn(context_stack *cs, object *sym, void (*fn)(context_stack *, long)) {
    // Put in slot 0. Function bindings should always be *global*
    object *o =  new_object(O_FN_NATIVE, fn);
    map_put(cs->cstack[0]->funcs, sym, o);
    object_set_name(o, strdup(string_ptr(oval_symbol(cs, sym))));
}

void bind_fn(context_stack *cs, object *sym, object *fn) {
    // Put in slot 0. Function bindings should always be *global*
    map_put(cs->cstack[0]->funcs, sym, fn);
}

void unbind_fn(context_stack *cs, object *sym) {
    map_delete(cs->cstack[0]->funcs, sym);
}

int sym_equal(void *a, void *b) {
    return a == b;
}

context *new_context() {
    context *c = malloc(sizeof (context));
    c->vars = map_create(sym_equal);
    c->funcs = map_create(sym_equal);
    return c;
}

context *top_context(context_stack *cs) {
    return cs->cstack[cs->cstackoff];
}

context *push_context(context_stack *cs) {
    context *c = new_context();
    push_existing_context(cs, c);
    return c;
}

context *push_existing_context(context_stack *cs, context *existing) {
    //printf("-------------------Pushing context: %p\n", existing);
    cs->cstackoff++;
    if(cs->cstackoff == cs->cstacksize) {
        cs->cstacksize *= 2;
        cs->cstack = realloc(cs->cstack, cs->cstacksize * sizeof (context *));
    }
    cs->cstack[cs->cstackoff] = existing;
    return existing;
}

context *pop_context(context_stack *cs) {
    context *c = cs->cstack[cs->cstackoff];
    cs->cstackoff--;
    //printf("-------------------Popping context: %p\n", c);
    return c;
}

size_t context_level(context_stack *cs) {
    return cs->cstackoff;
}

void pop_context_to_level(context_stack *cs, size_t level) {
    if(cs->cstackoff < level) {
        printf("CANNOT POP CONTEXT. TRIED TO GO TO LEVEL %ld BUT ALREADY AT LEVEL %ld\n",
               level, cs->cstackoff);
        PANIC("Lisp VM Died. context.c:122"); // This NEEDS to be an abort. the VM cannot recover.
    }
    cs->cstackoff = level;
}

void free_context(context *c) {
    map_destroy(c->vars);
    map_destroy(c->funcs);
    //memset(c, 0, sizeof (context));
    free(c);
}

struct context_var_iterator {
    context_stack *cs;
    size_t cstackoff;
    map_iterator *var_mi;
};

context_var_iterator *iterate_vars(context_stack *cs) {
    context_var_iterator *cvi = malloc(sizeof (struct context_var_iterator));
    cvi->cs = cs;
    cvi->cstackoff = cs->cstackoff;
    cvi->var_mi = iterate_map(cs->cstack[cvi->cstackoff]->vars);
    while(cvi->var_mi == NULL) {
        if(cvi->cstackoff == 0) {
            //memset(cvi, 0, sizeof (struct context_var_iterator));
            free(cvi);
            return NULL;
        }
        cvi->cstackoff--;
        cvi->var_mi = iterate_map(cs->cstack[cvi->cstackoff]->vars);
    }
    return cvi;
}

context_var_iterator *context_var_iterator_next(context_var_iterator *cvi) {
    map_iterator *next = map_iterator_next(cvi->var_mi);
    if(!next) {
        destroy_map_iterator(cvi->var_mi);
        cvi->var_mi = NULL;
        if(cvi->cstackoff > 0) {
            cvi->cstackoff--;
            cvi->var_mi = iterate_map(cvi->cs->cstack[cvi->cstackoff]->vars);
            while(cvi->var_mi == NULL) {
                if(cvi->cstackoff == 0) {
                    return NULL;
                }
                cvi->cstackoff--;
                cvi->var_mi = iterate_map(cvi->cs->cstack[cvi->cstackoff]->vars);
            }
            return cvi;
        }
        else {
            return NULL;
        }
    }
    cvi->var_mi = next;
    return cvi;
}

struct sym_val_pair context_var_iterator_values(context_var_iterator *cvi) {
    struct sym_val_pair svp;
    struct map_pair mp = map_iterator_values(cvi->var_mi);
       
    svp.sym = mp.key;
    svp.val = mp.val;
    return svp;
}

void destroy_context_var_iterator(context_var_iterator *cvi) {
    if(cvi->var_mi) {
        destroy_map_iterator(cvi->var_mi);
    }
    //memset(cvi, 0, sizeof (context_var_iterator));
    free(cvi);
}

struct context_fn_iterator {
    map_iterator *fn_mi;
};

context_fn_iterator *iterate_fns(context_stack *cs) {
    context_fn_iterator *cfi = malloc(sizeof (struct context_fn_iterator));
    cfi->fn_mi = iterate_map(cs->cstack[0]->funcs);
    while(cfi->fn_mi == NULL) {
        //memset(cfi, 0, sizeof (context_fn_iterator));
        free(cfi);
        return NULL;
    }
    return cfi;
}

context_fn_iterator *context_fn_iterator_next(context_fn_iterator *cfi) {
    map_iterator *next = map_iterator_next(cfi->fn_mi);
    if(!next) {
        destroy_map_iterator(cfi->fn_mi);
        cfi->fn_mi = NULL;
        return NULL;
    }
    cfi->fn_mi = next;
    return cfi;
}

struct sym_val_pair context_fn_iterator_values(context_fn_iterator *cfi) {
    struct sym_val_pair svp;
    struct map_pair mp = map_iterator_values(cfi->fn_mi);
       
    svp.sym = mp.key;
    svp.val = mp.val;
    return svp;
}

void destroy_context_fn_iterator(context_fn_iterator *cfi) {
    if(cfi->fn_mi) {
        destroy_map_iterator(cfi->fn_mi);
    }
    //memset(cfi, 0, sizeof (context_fn_iterator));
    free(cfi);    
}

map_t *context_vars(context *c) {
    return c->vars;
}

map_t *context_funcs(context *c) {
    return c->funcs;
}
