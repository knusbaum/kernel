#include "../stdio.h"
#include "../common.h"
#include "gc.h"
#include "object.h"
#include "threaded_vm.h"
#include "compiler.h"

#define INIT_STACK 4096

object **olist;
size_t o_off;
size_t o_size;

object **grey_queue;
size_t gq_off;
size_t gq_head;
size_t gq_size;

void push_olist(object *o) {
    if(o_off == o_size) {
        o_size *= 2;
        olist = realloc(olist, o_size * sizeof (object *));
    }
    olist[o_off++] = o;
}

void enqueue_grey_queue(object *o) {
    if(gq_off == gq_size) {
        gq_size *= 2;
        grey_queue = realloc(grey_queue, gq_size * sizeof (object *));
    }
    grey_queue[gq_off++] = o;
}

object *dequeue_grey_queue() {
    if(gq_head == gq_off) {
        return NULL;
    }
    return grey_queue[gq_head++];
}

void gc_init() {
    olist = malloc(sizeof (object *) * INIT_STACK);
    o_off = 0;
    o_size = INIT_STACK;
    enable_gc = 0;
}

//void *run_gc_loop(void *cs) {
//    while(1) {
//        //pthread_mutex_lock(get_gc_mut());
//        gc((context_stack *)cs);
//        //pthread_mutex_unlock(get_gc_mut());
//        sleep(1);
//    }
//    return NULL;
//}

int gci;
int enable_gc;
void gc(context_stack *cs) {
    gci++;
    if((gci % 1000) != 0) return;
    if(!enable_gc) return;
    //printf("\nStarting GC\n");
    grey_queue = malloc(sizeof (object *) * INIT_STACK);
    gq_off = 0;
    gq_head = 0;
    gq_size = INIT_STACK;

    //pthread_mutex_lock(get_gc_mut());
    size_t local_o_off = o_off;

    for(size_t i = 0; i < local_o_off; i++) {
        set_gc_flag(olist[i], GC_FLAG_WHITE);
    }

    struct sym_val_pair svp;

    context_var_iterator *cvi = iterate_vars(cs);
    context_var_iterator *curr_var = cvi;
    while(curr_var) {
        svp = context_var_iterator_values(cvi);
        set_gc_flag(svp.sym, GC_FLAG_GREY);
        set_gc_flag(svp.val, GC_FLAG_GREY);
        enqueue_grey_queue(svp.sym);
        enqueue_grey_queue(svp.val);
        curr_var = context_var_iterator_next(cvi);
    }
    destroy_context_var_iterator(cvi);

    context_fn_iterator *cfi = iterate_fns(cs);
    context_fn_iterator *curr_fn = cfi;
    while(curr_fn) {
        svp = context_fn_iterator_values(cfi);
        set_gc_flag(svp.sym, GC_FLAG_GREY);
        set_gc_flag(svp.val, GC_FLAG_GREY);
        enqueue_grey_queue(svp.sym);
        enqueue_grey_queue(svp.val);
        curr_fn = context_fn_iterator_next(cfi);
    }
    destroy_context_fn_iterator(cfi);

    object **stack = get_stack();
    for(size_t i = 0; i < get_stack_off(); i++) {
        set_gc_flag(stack[i], GC_FLAG_GREY);
        enqueue_grey_queue(stack[i]);
    }

    map_t *interned = get_interned();

    map_iterator *interned_it = iterate_map(interned);
    map_iterator *interned_curr = interned_it;
    while(interned_curr) {
        // INTERNED are always going to be just syms.
        // No sense scanning them.
        struct map_pair mp = map_iterator_values(interned_curr);
        if(gc_flag(mp.key) == GC_FLAG_WHITE) {
            set_gc_flag(mp.key, GC_FLAG_BLACK);
            //enqueue_grey_queue(mp.key);
        }
        if(gc_flag(mp.val) == GC_FLAG_WHITE) {
            set_gc_flag(mp.val, GC_FLAG_BLACK);
            //enqueue_grey_queue(mp.val);
        }
        interned_curr = map_iterator_next(interned_curr);
    }
    destroy_map_iterator(interned_it);

    map_t *internals = get_internals();
    map_iterator *internal_it = iterate_map(internals);
    map_iterator *internal_curr = internal_it;
    while(internal_curr) {
        struct map_pair mp = map_iterator_values(internal_curr);
        if(gc_flag(mp.key) == GC_FLAG_WHITE) {
            set_gc_flag(mp.key, GC_FLAG_GREY);
            enqueue_grey_queue(mp.key);
        }
        if(gc_flag(mp.val) == GC_FLAG_WHITE) {
            set_gc_flag(mp.val, GC_FLAG_GREY);
            enqueue_grey_queue(mp.val);
        }
        internal_curr = map_iterator_next(internal_curr);
    }
    destroy_map_iterator(internal_it);

    // We've checked all roots. Run through the grey ones and mark them.
    object *grey = dequeue_grey_queue();
    object *temp;
    compiled_chunk *cc;
    size_t i;
    map_iterator *it, *curr;
    struct map_pair mp;
    while(grey) {
        switch(otype(grey)) {
        case O_CHAR:
        case O_SYM:
        case O_STR:
        case O_NUM:
        case O_FN_NATIVE:
        case O_KEYWORD:
        case O_STACKOFFSET:
        case O_FSTREAM:
            break;
        case O_CONS:
            temp = ocar(NULL, grey);
            if(gc_flag(temp) == GC_FLAG_WHITE) {
                set_gc_flag(temp, GC_FLAG_GREY);
                enqueue_grey_queue(temp);
            }
            temp = ocdr(NULL, grey);
            if(gc_flag(temp) == GC_FLAG_WHITE) {
                set_gc_flag(temp, GC_FLAG_GREY);
                enqueue_grey_queue(temp);
            }
            break;
        case O_MACRO:
        case O_FN:
            temp = oval_fn_args(grey);
            if(gc_flag(temp) == GC_FLAG_WHITE) {
                set_gc_flag(temp, GC_FLAG_GREY);
                enqueue_grey_queue(temp);
            }
            temp = oval_fn_body(grey);
            if(gc_flag(temp) == GC_FLAG_WHITE) {
                set_gc_flag(temp, GC_FLAG_GREY);
                enqueue_grey_queue(temp);
            }
            break;
        case O_MACRO_COMPILED:
        case O_FN_COMPILED:
            cc = oval_fn_compiled(NULL, grey);
            for(i = 0; i < cc->b_off; i++) {
                if(cc->bs[i].has_arg && cc->bs[i].arg) {
                    if(gc_flag(cc->bs[i].arg) == GC_FLAG_WHITE) {
                        set_gc_flag(cc->bs[i].arg, GC_FLAG_GREY);
                        enqueue_grey_queue(cc->bs[i].arg);
                    }
                }
            }
            if(cc->c) {
                it = iterate_map(context_vars(cc->c));
                curr = it;
                while(curr) {
                    mp = map_iterator_values(curr);
                    if(gc_flag(mp.key) == GC_FLAG_WHITE) {
                        set_gc_flag(mp.key, GC_FLAG_GREY);
                        enqueue_grey_queue(mp.key);
                    }
                    if(gc_flag(mp.val) == GC_FLAG_WHITE) {
                        set_gc_flag(mp.val, GC_FLAG_GREY);
                        enqueue_grey_queue(mp.val);
                    }
                    curr = map_iterator_next(curr);
                }
                destroy_map_iterator(it);
            }
            break;
        }

        set_gc_flag(grey, GC_FLAG_BLACK);
        grey = dequeue_grey_queue();
    }

    // Actually free some stuff
    size_t new_olist_size = o_size;
    object **new_olist = malloc(new_olist_size * sizeof (object *));
    size_t new_olist_off = 0;
    for(size_t i = 0; i < o_off; i++) {
        if(gc_flag(olist[i]) == GC_FLAG_WHITE) {
//            printf("Freeing object: ");
//            print_object(olist[i]);
//            printf("@%x\n", olist[i]);
            destroy_object(olist[i]);
        }
        else {
            new_olist[new_olist_off++] = olist[i];
        }
    }

    free(olist);
    olist = new_olist;
    o_off = new_olist_off;
    o_size = new_olist_size;
    //pthread_mutex_unlock(get_gc_mut());

    free(grey_queue);
    gq_off = 0;
    gq_head = 0;
    gq_size = 0;
    //printf("Finishing GC.\n");
}

void add_object_to_gclist(object *o) {
    push_olist(o);
}

void dump_heap() {
    printf("\nDUMPING HEAP:\n--------------------------------------------------\n");
    for(size_t i = 0; i < o_off; i++) {
        print_object(olist[i]);
        printf("\n");
    }
}
