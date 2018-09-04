#ifndef THREADED_VM_H
#define THREADED_VM_H

//#include <pthread.h>
#include "../setjmp.h"
#include "object.h"
#include "context.h"
#include "map.h"

extern map_t *addrs;
extern map_t *special_syms;

void vm_init(context_stack *cs);

void compile_bytecode(compiled_chunk *cc, context_stack *cs, object *o);

map_t *get_vm_addrs();
void run_vm(context_stack *cs, compiled_chunk *cc);

// Should move these.
void push(object *o);
object *pop();
void dump_stack();
void call(context_stack *c, long variance);

object **get_stack();
size_t get_stack_off();

/****** Need Threads Impl ******/
//pthread_mutex_t *get_gc_mut();

void vm_error_impl(context_stack *cs, object *sym);
jmp_buf *vm_push_trap(context_stack *cs, object *sym);
void pop_trap();

#endif
