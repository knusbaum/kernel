#ifndef GC_H
#define GC_H

#include "object.h"
#include "context.h"

#define GC_FLAG_WHITE 1
#define GC_FLAG_GREY (1 << 1)
#define GC_FLAG_BLACK (1 << 2)

void gc_init();
void gc(context_stack *cs);
void *run_gc_loop(void *cs);
void add_object_to_gclist(object *o);

void dump_heap();

// quick and dirty hack to enable/disable GC.
extern int enable_gc;

#endif
