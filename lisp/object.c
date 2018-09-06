#include "../stdio.h"
#include "../common.h"
#include "object.h"
#include "compiler.h"
#include "map.h"
#include "gc.h"
#include "threaded_vm.h"

typedef struct cons cons;
struct cons {
    object *car;
    object *cdr;
};

object *car(cons *c) {
    return c->car;
}

void setcar(cons *c, object *o) {
    c->car = o;
}

object *cdr(cons *c) {
    return c->cdr;
}

void setcdr(cons *c, object *o) {
    c->cdr = o;
}

cons *new_cons() {
    cons *c = malloc(sizeof (cons));
    c->car = NULL;
    c->cdr = NULL;
    return c;
}

struct stream {
    char is_open;
};

struct file_stream {
    struct stream s;
    FILE *fstream;
};

struct object {
    char gcflag;
    enum obj_type type;
    union {
        string *str;
        cons *c;
        long num;
        void (*native)(void *, long);
        compiled_chunk *cc;
        struct file_stream *fstream;
        char character;
    };
    char *name;
};

char gc_flag(object *o) {
    return o->gcflag;
}

void set_gc_flag(object *o, char f) {
    o->gcflag = f;
}

void object_set_name(object *o, char *name) {
    o->name = name;
}

char *object_get_name(object *o) {
    return o->name;
}

object *new_object(enum obj_type t, void *o) {
    object *ob = malloc(sizeof (object));
    ob->gcflag = GC_FLAG_BLACK;
    add_object_to_gclist(ob);
    ob->type = t;
    ob->name = NULL;
    switch(t) {
    case O_KEYWORD:
    case O_SYM:
    case O_STR:
        ob->str = o;
        break;
    case O_STACKOFFSET:
    case O_NUM:
    case O_CHAR:
        PANIC("Cannot assign this type of object with this function.");
        break;
    case O_FN:
    case O_MACRO:
    case O_CONS:
        ob->c = o;
        break;
    case O_FN_NATIVE:
        ob->native = o;
        break;
    case O_FN_COMPILED:
        ob->cc = o;
        break;
    case O_MACRO_COMPILED:
        ob->cc = o;
        break;
    case O_FSTREAM:
        ob->fstream = o;
        break;
    }
    return ob;
}

object *new_object_cons(object *car, object *cdr) {
    cons *c = new_cons();
    setcar(c, car);
    setcdr(c, cdr);
    return new_object(O_CONS, c);
}

object *new_object_long(long l) {
    object *ob = malloc(sizeof (object));
    ob->gcflag = GC_FLAG_BLACK;
    add_object_to_gclist(ob);
    ob->type = O_NUM;
    ob->num = l;
    ob->name = NULL;
    return ob;
}

object *new_object_char(char c) {
    object *ob = malloc(sizeof (object));
    ob->gcflag = GC_FLAG_BLACK;
    add_object_to_gclist(ob);
    ob->type = O_CHAR;
    ob->character = c;
    ob->name = NULL;
    return ob;
}

object *new_object_stackoffset(long l) {
    object *ob = malloc(sizeof (object));
    ob->gcflag = GC_FLAG_BLACK;
    add_object_to_gclist(ob);
    ob->type = O_STACKOFFSET;
    ob->num = l;
    ob->name = NULL;
    return ob;
}

object *new_object_fn(object *args, object *body) {
    cons *cell = new_cons();
    setcar(cell, args);
    setcdr(cell, body);
    return new_object(O_FN, cell);
}

object *new_object_fn_compiled(compiled_chunk *cc) {
    return new_object(O_FN_COMPILED, cc);
}

object *new_object_macro(object *args, object *body) {
    cons *cell = new_cons();
    setcar(cell, args);
    setcdr(cell, body);
    return new_object(O_MACRO, cell);
}

object *new_object_macro_compiled(compiled_chunk *cc) {
    return new_object(O_MACRO_COMPILED, cc);
}

object *new_object_list(size_t len, ...) {
    va_list ap;
    va_start(ap, len);

    object *start = NULL;
    object *current = NULL;
    for(size_t i = 0; i < len; i++) {
        object *next = new_object_cons(va_arg(ap, object *), obj_nil());
        if(current) {
            setcdr(current->c, next);
            //osetcdr(current, next);
        }
        current = next;
        if(!start) start=current;
    }
    va_end(ap);
    return start;
}

enum obj_type otype(object *o) {
    return o->type;
}

const char *otype_str(enum obj_type t) {
    switch(t) {
    case O_SYM:
        return "symbol";
    case O_STR:
        return "string";
    case O_NUM:
        return "number";
    case O_CONS:
        return "cons cell";
    case O_FN:
        return "uncompiled fn";
    case O_FN_NATIVE:
        return "native fn";
    case O_KEYWORD:
        return "keyword";
    case O_MACRO:
        return "uncompiled macro";
    case O_FN_COMPILED:
        return "compiled fn";
    case O_MACRO_COMPILED:
        return "compiled macro";
    case O_STACKOFFSET:
        return "stack offset";
    case O_FSTREAM:
        return "file stream";
    case O_CHAR:
        return "character";
    }
    return "??? corrupt object";
}

string *oval_symbol(context_stack *cs, object *o) {
    if(o->type != O_SYM) {
        printf("Expected sym, but have: ");
        print_object(o);
        printf("\n");
//        abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return o->str;
}

string *oval_keyword(context_stack *cs, object *o) {
    if(o->type != O_KEYWORD) {
        printf("Expected keyword, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return o->str;
}

string *oval_string(context_stack *cs, object *o) {
    if(o->type != O_STR) {
        printf("Expected string, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return o->str;
}

long oval_long(context_stack *cs, object *o) {
    if(o->type != O_NUM) {
        printf("Expected number, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return o->num;
}

char oval_char(context_stack *cs, object *o) {
    if(o->type != O_CHAR) {
        printf("Expected character, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return o->character;
}

long oval_stackoffset(context_stack *cs, object *o) {
    if(o->type != O_STACKOFFSET) {
        printf("Expected stackoffset, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return o->num;
}

void (*oval_native(context_stack *cs, object *o))(void *, long) {
    if(o->type != O_FN_NATIVE) {
        printf("Expected number, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return o->native;
}

void (*oval_native_unsafe(object *o))(void *, long) {
    return o->native;
}

object *oval_fn_args(object *o) {
    cons *cell = o->c;
    return car(cell);
}

object *oval_fn_body(object *o) {
    cons *cell = o->c;
    return cdr(cell);
}

compiled_chunk *oval_fn_compiled(context_stack *cs, object *o) {
    if(o->type != O_FN_COMPILED && o->type != O_MACRO_COMPILED) {
        printf("Expected compiled function, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return o->cc;
}

compiled_chunk *oval_fn_compiled_unsafe(object *o) {
    return o->cc;
}

void oset_fn_compiled(object *o, compiled_chunk *cc) {
    o->cc = cc;
}

compiled_chunk *oval_macro_compiled(object *o) {
    return o->cc;
}

object *ocar(context_stack *cs, object *o) {
    if(o == obj_nil()) {
        return obj_nil();
    }
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return car(o->c);
}

object *ocdr(context_stack *cs, object *o) {
    if(o == obj_nil()) {
        return obj_nil();
    }
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    return cdr(o->c);
}

object *osetcar(context_stack *cs, object *o, object *car) {
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    setcar(o->c, car);
    return car;
}

object *osetcdr(context_stack *cs, object *o, object *cdr) {
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    setcdr(o->c, cdr);
    return cdr;
}

static void print_cons(cons *c);

static void print_cdr(object *o) {
    switch(otype(o)) {
    case O_KEYWORD:
    case O_SYM:
        printf(" . %s", string_ptr(o->str));
        break;
    case O_STR:
        printf(" . \"%s\"", string_ptr(o->str));
        break;
    case O_NUM:
        printf(" . %ld", o->num);
        break;
    case O_CONS:
        printf(" ");
        print_cons(o->c);
        break;
    case O_FN_NATIVE:
        printf(" . #<NATIVE FUNCTION (%s)>", o->name);
        break;
    case O_FN:
        printf(" . #<FUNCTION @ %p>", o);
        break;
    case O_MACRO:
        printf(" . #<MACRO @ %p>", o);
        break;
    case O_FN_COMPILED:
        printf(" . #<COMPILED FUNCTION @ %p>", o);
        break;
    case O_MACRO_COMPILED:
        printf(" . #<COMPILED MACRO @ %p>", o);
        break;
    case O_STACKOFFSET:
        printf(" . #<STACK_OFFSET: %ld>", o->num);
        break;
    case O_FSTREAM:
        printf(" . #<FILE STREAM>");
        break;
    case O_CHAR:
        printf(" . #\\%c", o->character);
        break;
    }
}

static void print_cons(cons *c) {
    object *o = car(c);
    if(o) {
        print_object(o);
        o = cdr(c);
        if(o != obj_nil()) {
            print_cdr(o);
        }
    }
}

static void print_list(cons *c) {
    printf("(");
    print_cons(c);
    printf(")");
}

void print_object(object *o) {
    if(!o) {
        printf("[[NULL]]");
        return;
    }
    switch(otype(o)) {
    case O_KEYWORD:
    case O_SYM:
        printf("%s", string_ptr(o->str));
        break;
    case O_STR:
        printf("\"%s\"", string_ptr(o->str));
        break;
    case O_NUM:
        printf("%ld", o->num);
        break;
    case O_CONS:
        print_list(o->c);
        break;
    case O_FN_NATIVE:
        printf("#<NATIVE FUNCTION %s>", o->name);
        break;
    case O_FN:
        printf("#<FUNCTION @ %p>", o);
        break;
    case O_MACRO:
        printf("#<MACRO @ %p>", o);
        break;
    case O_FN_COMPILED:
        printf("#<COMPILED FUNCTION @ %p>", o);
        break;
    case O_MACRO_COMPILED:
        printf("#<COMPILED MACRO @ %p>", o);
        break;
    case O_STACKOFFSET:
        printf("#<STACK_OFFSET: %ld>", o->num);
        break;
    case O_FSTREAM:
        printf("#<FILE STREAM>");
        break;
    case O_CHAR:
        printf("#\\%c", o->character);
        break;
    }
}

/** Stream Operations **/

object *new_object_fstream(context_stack *cs, string *fname, char *mode) {
    struct file_stream *fs = malloc(sizeof (struct file_stream));
    FILE *f = fopen(string_ptr(fname), mode);
    if(!f) {
        printf("Couldn't open file.\n");
        vm_error_impl(cs, interns("FILE-OPEN-ERROR"));
    }
    fs->fstream = f;
    fs->s.is_open = 1;
    return new_object(O_FSTREAM, fs);
}

object *new_object_fstream_unsafe(context_stack *cs, FILE *f) {
    (void)cs;
    struct file_stream *fs = malloc(sizeof (struct file_stream));
    fs->fstream = f;
    fs->s.is_open = 1;
    return new_object(O_FSTREAM, fs);
}

void fstream_close(context_stack *cs, object *o) {
    if(o->type != O_FSTREAM) {
        printf("Expected file stream, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    if(o->fstream->s.is_open) {
        fclose(o->fstream->fstream);
        o->fstream->s.is_open = 0;
    }
    o->fstream->fstream = NULL;
}

FILE *fstream_file(context_stack *cs, object *o) {
    if(o->type != O_FSTREAM) {
        printf("Expected file stream, but have: ");
        print_object(o);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("TYPE-ERROR"));
    }
    if(!o->fstream->fstream) {
        vm_error_impl(cs, interns("OPERATION-ON-CLOSED-STREAM-ERROR"));
    }
    return o->fstream->fstream;
}


/** Symbol Operations **/

map_t *symbols;

map_t *get_interned() {
    return symbols;
}

object *interns(char *symname) {
    return intern(new_string_copy(symname));
}

object *intern(string *symname) {
    if(!symbols) {
        symbols = map_create((int (*)(void *, void *))string_equal);
    }

    object *s = map_get(symbols, symname);
    if(!s) {
        if(string_ptr(symname)[0] == ':') {
            s = new_object(O_KEYWORD, symname);
        }
        else {
            s = new_object(O_SYM, symname);
        }
        s->str = symname;
        map_put(symbols, symname, s);
    }
    else {
        string_free(symname);
    }
    return s;
}

object *nil;
object *obj_nil() {
    if(!nil) {
        nil = interns("NIL");
    }
    return nil;
}

object *otrue;
object *obj_t() {
    if(!otrue) {
        otrue = interns("T");
    }
    return otrue;
}

void destroy_object(object *o) {
    switch(o->type) {
    case O_SYM:
    case O_STR:
    case O_KEYWORD:
        string_free(o->str);
        break;
    case O_NUM:
    case O_FN_NATIVE:
    case O_STACKOFFSET:
    case O_CHAR:
        break;
    case O_CONS:
    case O_FN:
    case O_MACRO:
        //memset(o->c, 0, sizeof (cons));
        free(o->c);
        break;
    case O_FN_COMPILED:
    case O_MACRO_COMPILED:
        //printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!!Freeing compiled function!!!!!!!!!!!!!!!!\n\n");
        free_compiled_chunk(o->cc);
        break;
    case O_FSTREAM:
        //printf("Destroying fstream.\n");
        if(o->fstream->fstream) {
            //printf("Closing unclosed FSTREAM.\n");
            fclose(o->fstream->fstream);
        }
        free(o->fstream);
        break;
    }
    //memset(o, 0, sizeof (object));
    free(o);
}
