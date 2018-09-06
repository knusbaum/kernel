#ifndef OBJECT_H
#define OBJECT_H

#include <stdarg.h>
#include "../stdio.h"
#include "lstring.h"
#include "map.h"

typedef struct object object;

//** NEED REFACTORING **/
typedef struct compiled_chunk compiled_chunk;
typedef struct context_stack context_stack;
//** END REFACTORING **/

enum obj_type {
    O_SYM,
    O_STR,
    O_NUM,
    O_CONS,
    O_FN,
    O_FN_NATIVE,
    O_KEYWORD,
    O_MACRO,
    O_FN_COMPILED,
    O_MACRO_COMPILED,
    O_STACKOFFSET,
    O_FSTREAM,
    O_CHAR
};

void object_set_name(object *o, char *name);

/** Object Operations **/
object *new_object(enum obj_type t, void *o);
object *new_object_cons(object *car, object *cdr);
object *new_object_long(long l);
object *new_object_char(char c);
object *new_object_stackoffset(long l);
object *new_object_fn(object *args, object *body);
object *new_object_fn_compiled(compiled_chunk *cc);
object *new_object_macro(object *args, object *body);
object *new_object_macro_compiled(compiled_chunk *cc);
object *new_object_list(size_t len, ...);
enum obj_type otype(object *o);
const char *otype_str(enum obj_type);

string *oval_symbol(context_stack *cs, object *o);
string *oval_keyword(context_stack *cs, object *o);
string *oval_string(context_stack *cs, object *o);
long oval_long(context_stack *cs, object *o);
char oval_char(context_stack *cs, object *o);
long oval_stackoffset(context_stack *cs, object *o);
void (*oval_native(context_stack *cs, object *o))(void *, long);
void (*oval_native_unsafe(object *o))(void *, long);
object *oval_fn_args(object *o); // Also for getting macro args
object *oval_fn_body(object *o); // Also for getting macro body
compiled_chunk *oval_fn_compiled(context_stack *cs, object *o);
compiled_chunk *oval_fn_compiled_unsafe(object *o);
void oset_fn_compiled(object *o, compiled_chunk *cc);
compiled_chunk *oval_macro_compiled(object *o);
object *ocar(context_stack *cs, object *o);
object *ocdr(context_stack *cs, object *o);
object *osetcar(context_stack *cs, object *o, object *car);
object *osetcdr(context_stack *cs, object *o, object *cdr);
void print_object(object *o);

/** Stream Operations **/
object *new_object_fstream(context_stack *cs, string *fname, char *mode);
object *new_object_fstream_unsafe(context_stack *cs, FILE *f); // Used for bootstrapping. Unsafe since we don't know state of f.
void fstream_close(context_stack *cs, object *o);
FILE *fstream_file(context_stack *cs, object *o);

/** Symbol Operations **/
object *interns(char *symname);
object *intern(string *symname);
map_t *get_interned();

/** Important Objects **/
object *obj_nil();
object *obj_t();

char gc_flag(object *o);
void set_gc_flag(object *o, char f);

void destroy_object(object *o);

#endif
