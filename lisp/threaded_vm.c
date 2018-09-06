#include "../stdio.h"
#include "../common.h"
#include "threaded_vm.h"
#include "context.h"
#include "map.h"
#include "compiler.h"
#include "gc.h"

static int str_eq(void *s1, void *s2) {
    return strcmp((char *)s1, (char *)s2) == 0;
}

map_t *addrs;
//map_t *get_vm_addrs();

//pthread_mutex_t gc_mut;
//pthread_mutex_t *get_gc_mut() {
//    return &gc_mut;
//}

map_t *special_syms;

#define INIT_STACK 4096
object **stack;
size_t s_off;
size_t stack_size;

object **get_stack() {
    return stack;
}

size_t get_stack_off() {
    return s_off;
}

/* error handling machinery */
/**
 * I don't think we need to scan this in GC, since the syms
 * should be interned, and the list of interned syms is a
 * GC root.
 */
struct stack_trap {
    jmp_buf buff;
    size_t s_off;
    size_t trap_stack_off;
    size_t context_off;
    object *catcher;
};

#define INIT_TRAP_STACK 64
struct stack_trap *trap_stack;
size_t trap_stack_off;
size_t trap_stack_size;

static inline jmp_buf *__push_trap(context_stack *cs, object *catcher) {
    if(trap_stack_off == trap_stack_size) {
        trap_stack_size *= 2;
        stack = realloc(trap_stack, trap_stack_size * sizeof (object *));
    }
    trap_stack[trap_stack_off].s_off = s_off;
    trap_stack[trap_stack_off].trap_stack_off = trap_stack_off;
    trap_stack[trap_stack_off].context_off = context_level(cs);
    trap_stack[trap_stack_off].catcher = catcher;
    jmp_buf *ret = &trap_stack[trap_stack_off].buff;
    trap_stack_off++;
    return ret;
}

static inline void __pop_trap() {
    if(trap_stack_off == 0) return;
    trap_stack[trap_stack_off].catcher = NULL;
    --trap_stack_off;
}

void pop_trap() {
    __pop_trap();
}

static inline void vm_mult(context_stack *cs, long variance);
static inline void vm_div(context_stack *cs, long variance);
void vm_num_eq(context_stack *cs, long variance);
void vm_num_gt(context_stack *cs, long variance);
void vm_num_lt(context_stack *cs, long variance);
void vm_list(context_stack *cs, long variance);
void vm_append(context_stack *cs, long variance);
void vm_splice(context_stack *cs, long variance);
void vm_car(context_stack *cs, long variance);
void vm_cdr(context_stack *cs, long variance);
void vm_cons(context_stack *cs, long variance);
void vm_length(context_stack *cs, long variance);
void vm_eq(context_stack *cs, long variance);
void vm_compile(context_stack *cs, long variance);
void vm_compile_fn(context_stack *cs, long variance);
void vm_compile_lambda(context_stack *cs, long variance);
void vm_compile_macro(context_stack *cs, long variance);
void vm_eval(context_stack *cs, long variance);
void vm_read(context_stack *cs, long variance);
void vm_print(context_stack *cs, long variance);
void vm_macroexpand(context_stack *cs, long variance);
void vm_gensym(context_stack *cs, long variance);
void vm_error(context_stack *cs, long variance);
void vm_open(context_stack *cs, long variance);
void vm_close(context_stack *cs, long variance);
void vm_read_char(context_stack *cs, long variance);
void vm_str_nth(context_stack *cs, long variance);
void vm_str_len(context_stack *cs, long variance);

parser *stdin_parser;

void vm_init(context_stack *cs) {
//    pthread_mutex_init(&gc_mut, NULL); //fastmutex; //PTHREAD_MUTEX_INITIALIZER;
//    pthread_mutex_lock(&gc_mut);

    stack = malloc(sizeof (object *) * INIT_STACK);
    s_off = 0;
    stack_size = INIT_STACK;

    trap_stack = malloc(sizeof (struct stack_trap) * INIT_TRAP_STACK);
    trap_stack_off = 0;
    trap_stack_size = INIT_TRAP_STACK;

    bind_var(cs, interns("NIL"), interns("NIL"));
    bind_var(cs, interns("T"), interns("T"));

    bind_native_fn(cs, interns(">"), vm_num_gt);
    bind_native_fn(cs, interns("<"), vm_num_lt);
    bind_native_fn(cs, interns("LIST"), vm_list);
    bind_native_fn(cs, interns("APPEND"), vm_append);
    bind_native_fn(cs, interns("SPLICE"), vm_splice);
    bind_native_fn(cs, interns("CAR"), vm_car);
    bind_native_fn(cs, interns("CDR"), vm_cdr);
    bind_native_fn(cs, interns("CONS"), vm_cons);
    bind_native_fn(cs, interns("LENGTH"), vm_length);
    bind_native_fn(cs, interns("EQ"), vm_eq);
    bind_native_fn(cs, interns("COMPILE-FN"), vm_compile_fn);
    bind_native_fn(cs, interns("COMPILE-LAMBDA"), vm_compile_lambda);
    bind_native_fn(cs, interns("COMPILE-MACRO"), vm_compile_macro);
    bind_native_fn(cs, interns("EVAL"), vm_eval);
    bind_native_fn(cs, interns("READ"), vm_read);
    bind_native_fn(cs, interns("PRINT"), vm_print);
    bind_native_fn(cs, interns("MACROEXPAND"), vm_macroexpand);
    bind_native_fn(cs, interns("GENSYM"), vm_gensym);
    bind_native_fn(cs, interns("ERROR"), vm_error);
    bind_native_fn(cs, interns("OPEN"), vm_open);
    bind_native_fn(cs, interns("CLOSE"), vm_close);
    bind_native_fn(cs, interns("READ-CHAR"), vm_read_char);
    bind_native_fn(cs, interns("STR-NTH"), vm_str_nth);
    bind_native_fn(cs, interns("STR-LEN"), vm_str_len);

    addrs = get_vm_addrs();
    special_syms = map_create(sym_equal);
    map_put(special_syms, obj_t(), (void *)1);
    map_put(special_syms, obj_nil(), (void *)1);

    stdin_parser = new_parser_file(stdino);
}

static inline void __push(object *o) {
    if(s_off == stack_size) {
        stack_size *= 2;
        stack = realloc(stack, stack_size * sizeof (object *));
    }
    //printf("Pushing: ");
    //print_object(o);
    //printf("\n");
    stack[s_off++] = o;
    //dump_stack();
}

static inline object *__pop() {
    if(s_off == 0) return obj_nil();
    //printf("Popping: \n");
    //dump_stack();
    return stack[--s_off];
}

static inline object *__top() {
    if(s_off == 0) return obj_nil();
    return stack[s_off - 1];
}

void push(object *o) {
    __push(o);
}

object *pop() {
    return __pop();
}

void dump_stack() {
    for(size_t i = 0; i < s_off; i++) {
        print_object(stack[i]);
        printf("\n");
    }
    printf("\n---------------------\n");
}

static inline void vm_mult(context_stack *cs, long variance) {
    (void)cs;
    long val = 1;
    for(int i = 0; i < variance; i++) {
        val *= oval_long(cs, __pop());
    }
    __push(new_object_long(val));
}

static inline void vm_div(context_stack *cs, long variance) {
    if(variance < 1) {
        printf("Expected at least 1 argument, but got none.\n");
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    (void)cs;
    long val = 1;
    printf("Signalling error: ");
    print_object(__top());
    printf("\n");
    for(int i = 0; i < variance - 1; i++) {
        val *= oval_long(cs, __pop());
    }
    val = oval_long(cs, __pop()) / val;
    __push(new_object_long(val));
}

void vm_num_eq(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    if(oval_long(cs, __pop()) == oval_long(cs, __pop())) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
}

void vm_num_gt(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        interns("SIG-ERROR");
    }
    // args are reversed.
    if(oval_long(cs, __pop()) < oval_long(cs, __pop())) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
}

void vm_num_lt(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    // args are reversed.
    if(oval_long(cs, __pop()) > oval_long(cs, __pop())) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
}

void vm_list(context_stack *cs, long variance) {
    (void)cs;
    object *cons = obj_nil();
    for(int i = 0; i < variance; i++) {
        cons = new_object_cons(__pop(), cons);
    }
    __push(cons);
}

void vm_append(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }

    (void)cs;
    object *cons = new_object_cons(__pop(), obj_nil());
    object *target = __pop();
    if(target == obj_nil()) {
        __push(cons);
        return;
    }
    object *curr = target;
    while(ocdr(cs, curr) != obj_nil()) {
        curr=ocdr(cs, curr);
    }
    osetcdr(cs, curr, cons);
    __push(target);
}

void vm_splice(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }

    (void)cs;
    object *to_splice = __pop();
    object *target = __pop();
    if(target == obj_nil()) {
        __push(to_splice);
        return;
    }
    object *curr = target;
    while(ocdr(cs, curr) != obj_nil()) {
        curr=ocdr(cs, curr);
    }
    osetcdr(cs, curr, to_splice);
    __push(target);
}

void vm_car(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *list = __pop();
    __push(ocar(cs, list));
}

void vm_cdr(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *list = __pop();
    __push(ocdr(cs, list));
}

void vm_cons(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *cdr = __pop();
    object *car = __pop();
    object *cons = new_object_cons(car, cdr);
    __push(cons);
}

void vm_length(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }

    long len = 0;
    for(object *curr = __pop(); curr != obj_nil(); curr = ocdr(cs, curr)) {
        len++;
    }
    __push(new_object_long(len));
}

void vm_eq(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    if(__pop() == __pop()) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
}

void vm_compile_fn(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *fname = __pop();
    object *uncompiled_fn = __pop();
    compiled_chunk *fn_cc = new_compiled_chunk();
    fn_cc->c = top_context(cs);
    object *fn = new_object_fn_compiled(fn_cc);
    bind_fn(cs, fname, fn);
    //push(fn);

    // Unbind the fn if anything goes wrong.
    jmp_buf *trap = vm_push_trap(cs, obj_nil());
    int ret = setjmp(*trap);
    if(ret) {
        printf("ERROR IN VM_COMPILE_FN!\n");
        //free_compiled_chunk(cc);
        unbind_fn(cs, fname);
        vm_error_impl(cs, __pop());
        return;
    }
    compile_fn(fn_cc, cs, uncompiled_fn);
    __pop_trap();
}

void vm_compile_lambda(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    //object *fname = __pop();
    object *uncompiled_fn = __pop();
    compiled_chunk *fn_cc = new_compiled_chunk();
    fn_cc->c = top_context(cs);
    object *fn = new_object_fn_compiled(fn_cc);
    //bind_fn(cs, fname, fn);

    // Unbind the fn if anything goes wrong.
    jmp_buf *trap = vm_push_trap(cs, obj_nil());
    int ret = setjmp(*trap);
    if(ret) {
        printf("ERROR IN VM_COMPILE_LAMBDA!\n");
        //free_compiled_chunk(cc);
        //unbind_fn(cs, fname);
        vm_error_impl(cs, __pop());
        return;
    }
    compile_fn(fn_cc, cs, uncompiled_fn);
    __pop_trap();
    //printf("Creating lambda at %p\n", fn);
    push(fn);
}

void vm_compile_macro(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *fname = __pop();
    object *uncompiled_fn = __pop();
    compiled_chunk *fn_cc = new_compiled_chunk();
    fn_cc->c = top_context(cs);
    object *macro = new_object_macro_compiled(fn_cc);
    bind_fn(cs, fname, macro);
    compile_fn(fn_cc, cs, uncompiled_fn);
}

void call(context_stack *cs, long variance) {
    object *fn = __pop();
    if(fn == obj_nil() || fn == NULL) {
        printf("Cannot call function: ");
        print_object(fn);
        printf("\n");
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    if (otype(fn) == O_FN_COMPILED) {
        compiled_chunk *cc = oval_fn_compiled(cs, fn);
        if(cc->c) {
            push_existing_context(cs, cc->c);
        }
        run_vm(cs, cc);
        set_gc_flag(__top(), GC_FLAG_BLACK);
        object *ret = __pop();
        for(int i = 0; i < variance; i++) {
            __pop();
        }
        //__pop(); // pop fn
        __push(ret);
        if(cc->c) {
            pop_context(cs);
        }
    }
    else if(otype(fn) == O_FN_NATIVE) {
        oval_native_unsafe(fn)(cs, variance);
    }
    else if(otype(fn) == O_MACRO) {
        compiled_chunk *cc = oval_macro_compiled(fn);
        object *result = __pop();
        compile_bytecode(cc, cs, result);
    }
    else {
        printf("Cannot eval this thing. not implemented: ");
        print_object(fn);
        printf("\n");
        dump_stack();
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
}

void resolve(context_stack *cs) {
    object *sym = __pop();
    object *val = lookup_var(cs, sym);
    if(val) {
        __push(val);
    }
    else {
        printf("Error: Symbol %s is not bound.\n", string_ptr(oval_symbol(cs, sym)));
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
}

void vm_macroexpand_rec(context_stack *cs, long rec) {
    object *o = __top();
    if(rec >= 4096) {
        printf("Cannot expand macro. Nesting too deep.\n");
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    rec++;
    if(otype(o) == O_CONS) {
        object *fsym = ocar(cs, o);
        object *func = lookup_fn(cs, fsym);

        if(func && otype(func) == O_MACRO_COMPILED) {
            // Don't eval the arguments.
            long num_args = 0;
            for(object *margs = ocdr(cs, o); margs != obj_nil(); margs = ocdr(cs, margs)) {
                push(ocar(cs, margs));
                num_args++;
            }

            compiled_chunk *fn_cc = oval_fn_compiled(cs, func);
            if(num_args > fn_cc->variance) {
                if(fn_cc->flags & CC_FLAG_HAS_REST) {
                    push(lookup_fn(cs, interns("LIST")));
                    call(cs, num_args - fn_cc->variance);
                    num_args = fn_cc->variance + 1;
                }
                else {
                    printf("Expected exactly %ld arguments, but got %ld.\n", fn_cc->variance, num_args);
                    //abort();
                    vm_error_impl(cs, interns("SIG-ERROR"));
                }
            }

            compiled_chunk *func_cc = oval_fn_compiled(cs, func);
            run_vm(cs, func_cc);

            object *exp = pop();
            for(int i = 0; i < num_args; i++) {
                pop();
            }
            pop();
            push(exp);
            vm_macroexpand_rec(cs, rec);
        }
        else {
            for(object *margs = o; margs != obj_nil(); margs = ocdr(cs, margs)) {
                push(ocar(cs, margs));
                vm_macroexpand_rec(cs, rec);
                set_gc_flag(__top(), GC_FLAG_BLACK);
                object *o = pop();
                osetcar(cs, margs, o);
            }
            return;
        }
    }
    else {
        return;
    }
}

void vm_macroexpand(context_stack *cs, long variance) {
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    return vm_macroexpand_rec(cs, 0);
}

unsigned int gensym_num;

void vm_gensym(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 0) {
        printf("Expected exactly 0 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }

    char *gensym = malloc(18); // 10 digits + "gensym-"(7) + \0
    sprintf(gensym, "gensym-%d", gensym_num++);
    object *new_gensym = interns(gensym);
    free(gensym);
    __push(new_gensym);
}

void vm_error(context_stack *cs, long variance) {
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
//    printf("Calling vm_error(%ld).\n", trap_stack_off);
//    printf("Dumping stack in vm_error_impl:\n");
//    dump_stack();
    object *sym = pop();
    vm_error_impl(cs, sym);
}
void vm_error_impl(context_stack *cs, object *sym) {

    for(ssize_t i = trap_stack_off - 1; i >= 0; i--) {
//        printf("Checking trap_stack[%lu].\n", i);
//        printf("sym: ");
//        print_object(sym);
//        printf(", catcher: ");
//        print_object(trap_stack[i].catcher);
//        printf("\n");
        if(sym == trap_stack[i].catcher
           || trap_stack[i].catcher == obj_nil()) {
            if(!cs) {
                printf("Cannot handle error without context_stack.\n");
                PANIC("Cannot handle error without context_stack.");
            }

//            printf("%ld Current s_off: %ld, after trap_stack_off: %ld\n", i, s_off, trap_stack[i].s_off);
//            printf("Longjmping for error: ");
//            print_object(sym);
//            printf("\nStack:\n");
//            dump_stack();

            // Someone wants to catch this error.
            pop_context_to_level(cs, trap_stack[i].context_off);
            trap_stack_off = trap_stack[i].trap_stack_off;
            s_off = trap_stack[i].s_off;
            push(sym);
            //printf("Afterward: \n");
            //dump_stack();
            longjmp(trap_stack[i].buff, 3);
        }
    }

    printf(" FAILED TO CATCH ERROR: ");
    print_object(sym);
    printf(" ABORTING!\n");
    PANIC("UNCAUGHT LISP VM EXCEPTION."); // This has to be an abort. We can't continue.
}

jmp_buf *vm_push_trap(context_stack *cs, object *sym) {
    jmp_buf *buff = __push_trap(cs, sym);
    return buff;
}

void vm_open(context_stack *cs, long variance) {
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *fname = __pop();
    __push(new_object_fstream(cs, oval_string(cs, fname), "r+"));
}

void vm_close(context_stack *cs, long variance) {
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    fstream_close(cs, __pop());
    __push(obj_nil());
}

void vm_read_char(context_stack *cs, long variance) {
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *o = pop();
    FILE *f = fstream_file(cs, o);
    int c = getc(f);
    if(c == EOF) {
        vm_error_impl(cs, interns("END-OF-FILE"));
    }
    __push(new_object_char(c));
}

void vm_str_nth(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *elem = pop();
    object *str = pop();
    string *lstr = oval_string(cs, str);
    size_t celem = oval_long(cs, elem);
    if(celem >= string_len(lstr)) {
        vm_error_impl(cs, interns("ARRAY-OUT-OF-BOUNDS"));
    }
    int c = string_ptr(lstr)[celem];
    __push(new_object_char(c));
}

void vm_str_len(context_stack *cs, long variance) {
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    object *str = pop();
    string *lstr = oval_string(cs, str);
    long len = string_len(lstr) - 1;
    __push(new_object_long(len));
}

void vm_eval(context_stack *cs, long variance) {
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    push(lookup_fn(cs, interns("MACROEXPAND")));
    call(cs, 1);

    compiled_chunk *cc = new_compiled_chunk();

    // We need to catch any errors to deallocate the chunk and rethrow.
    jmp_buf *trap = vm_push_trap(cs, obj_nil());
    int ret = setjmp(*trap);
    if(ret) {
        printf("ERROR IN VM_EVAL! THROWING SIG_ERROR\n");
        //free_compiled_chunk(cc);
        vm_error_impl(cs, __pop());
    }

    object *o = __pop();
//    printf("Macroexpanded: ");
//    print_object(o);
//    printf("\n");
    compile_form(cc, cs, o);
    run_vm(cs, cc);
    free_compiled_chunk(cc);
    __pop_trap();
}

void vm_read(context_stack *cs, long variance) {
    (void)cs;
    if(variance > 1) {
        printf("Expected 0 or 1 arguments, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    parser *p = stdin_parser;
    if(variance == 1) {
        //printf("Read dumping stack: \n");
        //dump_stack();
        object *o = pop();
        //printf("Creating parser from object: ");
        //print_object(o);
        //printf("@%x\n", o);
        p = new_parser_file(fstream_file(cs, o));
    }
    object *o = next_form(p, cs);
    if(o) {
        __push(o);
    }
    else {
        __push(obj_nil());
    }
    if(variance == 1) {
        destroy_parser(p);
    }
}

void vm_print(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        //abort();
        vm_error_impl(cs, interns("SIG-ERROR"));
    }
    print_object(__pop());
    __push(obj_nil());
}

//        dump_stack();
//        printf("--------------------------\n");

#define NEXTI {                                 \
        bs++;                                   \
        goto *bs->instr;                        \
    };

static inline void *___vm(context_stack *cs, compiled_chunk *cc, int _get_vm_addrs) {
    if(cs && context_level(cs) > 20000)
        vm_error_impl(cs, interns("STACK-OVERFLOW"));

    if(_get_vm_addrs) {
        map_t *m = map_create(str_eq);
        map_put(m, "chew_top", &&chew_top);
        map_put(m, "push", &&push);
        map_put(m, "pop", &&pop);
        map_put(m, "call", &&call);
        map_put(m, "resolve_sym", &&resolve_sym);
        map_put(m, "bind_var", &&bind_var);
        map_put(m, "bind_fn", &&bind_fn);
        map_put(m, "push_lex_context", &&push_lex_context);
        map_put(m, "pop_lex_context", &&pop_lex_context);
        map_put(m, "go", &&go);
        map_put(m, "go_if_nil", &&go_if_nil);
        map_put(m, "go_if_not_nil", &&go_if_not_nil);
        map_put(m, "push_from_stack", &&push_from_stack);
        map_put(m, "add", &&add);
        map_put(m, "subtract", &&subtract);
        map_put(m, "multiply", &&multiply);
        map_put(m, "divide", &&divide);
        map_put(m, "num_eq", &&num_eq);
        map_put(m, "catch", &&catch);
        map_put(m, "pop_catch", &&pop_catch);
        map_put(m, "pop_to_stack", &&pop_to_stack);
        map_put(m, "exit", &&exit);
        map_put(m, "save_stackoff", &&save_stackoff);
        map_put(m, "restore_stackoff", &&restore_stackoff);
        return m;
    }

//    printf("push: (%p)\n", &&push);
//    printf("pop: (%p)\n", &&pop);
//    printf("call: (%p)\n", &&call);
//    printf("resolve_sym: (%p)\n", &&resolve_sym);
//    printf("bind: (%p)\n", &&bind);
//    printf("push_lex_context: (%p)\n", &&push_lex_context);
//    printf("pop_lex_context: (%p)\n", &&pop_lex_context);
//    printf("go: (%p)\n", &&go);
//    printf("go_if_nil: (%p)\n", &&go_if_nil);
//    printf("exit: (%p)\n", &&exit);

    struct binstr *bs = cc->bs;
    struct binstr *bs_saved;
    volatile size_t saved_stackoff;

    goto *bs->instr;
    return NULL;

    size_t target;
    object *ret, *sym, *val;
    long mathvar;
    long truthiness;
    int i;
    jmp_buf *jmp;
chew_top:
    //printf("%ld@%p, CHEW_TOP (%ld): ", bs - cc->bs, cc, bs->offset);
    ret = __pop();
    s_off -= bs->offset;
    __push(ret);
    NEXTI;
push:
    //printf("%ld@%p, PUSH: ", bs - cc->bs, cc);
    //print_object(bs->arg);
    //printf("\n");
    __push(bs->arg);
    //dump_stack();
    NEXTI;
pop:
    //printf("%ld@%p POP: ", bs - cc->bs, cc);
    ret = __pop();
    //print_object(ret);
    //printf("\n");
    //dump_stack();
    NEXTI;
call:
    //printf("%ld@%p CALL (%ld)\n", bs - cc->bs, cc, bs->variance);
    call(cs, bs->variance);
    //dump_stack();
    NEXTI;
resolve_sym:
    //printf("%ld@%p RESOLVE_SYM\n", bs - cc->bs, cc);
    resolve(cs);
    NEXTI;
bind_var:
    //printf("%ld@%p BIND_VAR\n", bs - cc->bs, cc);
    sym = __pop();
    val = __pop();
    bind_var(cs, sym, val);
    NEXTI;
bind_fn:
    sym = __pop();
    val = __pop();
    //printf("%ld@%p BIND_FN\n", bs - cc->bs, cc);
    bind_fn(cs, sym, val);
    NEXTI;
push_lex_context:
    //printf("%ld@%p PUSH_LEX_CONTEXT %p\n", bs - cc->bs, cc, top_context(cs));
    push_context(cs);
    NEXTI;
pop_lex_context:
    //printf("%ld@%p POP_LEX_CONTEXT: %p\n", bs - cc->bs, cc, top_context(cs));
    pop_context(cs);
    NEXTI;
go:
    target = (size_t)map_get(cc->labels, bs->str);
    //printf("%ld@%p (%p)GO (%s)(%ld)\n", bs - cc->bs, cc, bs, bs->str, target);
    bs->instr = &&go_optim;
    bs->offset = target;
    bs = cc->bs + target;
    goto *bs->instr;
go_if_nil:
    target = (size_t)map_get(cc->labels, bs->str);
    //printf("%ld@%p (%p)GO_IF_NIL (%s)(%ld) ", bs - cc->bs, cc, bs, bs->str, target);
    bs->instr = &&go_if_nil_optim;
    bs->offset = target;
    if(__pop() == obj_nil()) {
        //printf("(jumping to %p)\n", (cc->bs + target)->instr);
        bs = cc->bs + target;
        goto *bs->instr;
    }
    //printf("(not jumping)\n");
    NEXTI;
go_if_not_nil:
    target = (size_t)map_get(cc->labels, bs->str);
    //printf("%ld@%p (%p)GO_IF_NOT_NIL (%s)(%ld) ", bs - cc->bs, cc, bs, bs->str, target);
    bs->instr = &&go_if_not_nil_optim;
    bs->offset = target;
    if(__pop() != obj_nil()) {
        //printf("(jumping to %p)\n", (cc->bs + target)->instr);
        bs = cc->bs + target;
        goto *bs->instr;
    }
    //printf("(not jumping)\n");
    NEXTI;
go_optim:
    //printf("%ld@%p GO_OPTIM (%ld)\n", bs - cc->bs, cc, bs->offset);
    bs = cc->bs + bs->offset;
    goto *bs->instr;
go_if_nil_optim:
    //printf("%ld@%p GO_IF_NIL_OPTIM (%ld) ", bs - cc->bs, cc, bs->offset);
    if(__pop() == obj_nil()) {
        //printf("(jumping)\n");
        bs = cc->bs + bs->offset;
        goto *bs->instr;
    }
    //printf("(not jumping)\n");
    NEXTI;
go_if_not_nil_optim:
    //printf("%ld@%p GO_IF_NOT_NIL_OPTIM (%ld) ", bs - cc->bs, cc, bs->offset);
    if(__pop() != obj_nil()) {
        //printf("(jumping)\n");
        bs = cc->bs + bs->offset;
        goto *bs->instr;
    }
    //printf("(not jumping)\n");
    NEXTI;
push_from_stack:
    //printf("%ld@%p PUSH_FROM_STACK (%ld)\n", bs - cc->bs, cc, s_off - 1 - bs->offset);
    //dump_stack();
    __push(stack[s_off - 1 - bs->offset]);
    NEXTI;
add:
    //printf("%ld@%p ADD (%ld)\n", bs - cc->bs, cc, bs->variance);
    mathvar = 0;
    for(i = 0; i < bs->variance; i++) {
        mathvar += oval_long(cs, __top());
        __pop();
    }
    __push(new_object_long(mathvar));
    NEXTI;
subtract:
    //printf("%ld@%p SUBTRACT (%ld)\n", bs - cc->bs, cc, bs->variance);
    mathvar = 0;
    for(i = 0; i < bs->variance - 1; i++) {
        mathvar += oval_long(cs, __pop());
    }
    mathvar = oval_long(cs, __pop()) - mathvar;
    __push(new_object_long(mathvar));
    NEXTI;
multiply:
    //printf("%ld@%p MULTIPLY\n", bs - cc->bs, cc);
    vm_mult(cs, bs->variance);
    NEXTI;
divide:
    //printf("%ld@%p DIVIDE\n", bs - cc->bs, cc);
    vm_div(cs, bs->variance);
    NEXTI;
num_eq:
    //printf("%ld@%p NUM_EQ\n", bs - cc->bs, cc);
    truthiness = 1;
    mathvar = oval_long(cs, __pop());
    for(long i = 0; i < bs->variance - 1; i++) {
        truthiness = truthiness && (mathvar == oval_long(cs, __pop()));
    }
    if(truthiness) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
    NEXTI;
catch:
    //printf("%ld@%p CATCH ", bs - cc->bs, cc);
    bs_saved = bs;
    jmp = vm_push_trap(cs, pop());
    i = setjmp(*jmp);
    if(i) {
        //printf("PUSHING T\n");
        push(obj_t());
        bs = bs_saved;
    }
    else {
        //printf("PUSHING NIL\n");
        push(obj_nil());
    }
    NEXTI;
pop_catch:
    //printf("%ld@%p POP_CATCH\n", bs - cc->bs, cc);
    pop_trap();
    NEXTI;
pop_to_stack:
    //printf("%ld@%p POP_TO_STACK (%ld) (s_off %ld)\n", bs - cc->bs, cc, s_off - 1 - bs->offset, s_off);
    if(s_off - 1 == s_off - 1 - bs->offset) {
        //printf("SKIPPING!\n");
    }
    else {
        stack[s_off - 1 - bs->offset] = __top();
        //dump_stack();
        __pop();
        //dump_stack();
    }
    NEXTI;
save_stackoff:
    //printf("%ld@%p SAVE_STACKOFF\n", bs - cc->bs, cc);
    //dump_stack();
    saved_stackoff = s_off;
    NEXTI;
restore_stackoff:
    //printf("%ld@%p RESTORE_STACKOFF\n", bs - cc->bs, cc);
    s_off = saved_stackoff;
    //dump_stack();
    NEXTI;
exit:
    //printf("%ld@%p EXIT\n", bs - cc->bs, cc);
    //dump_stack();
//    pthread_mutex_unlock(&gc_mut);
//    pthread_mutex_lock(&gc_mut);
    gc(cs);
    return NULL;
}

map_t *get_vm_addrs() {
    return (map_t *)___vm(NULL, NULL, 1);
}

void run_vm(context_stack *cs, compiled_chunk *cc) {
    ___vm(cs, cc, 0);
}
