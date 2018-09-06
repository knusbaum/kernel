#include "../stdio.h"
#include "../setjmp.h"
#include "../common.h"
#include "lexer.h"
#include "parser.h"
#include "context.h"
#include "threaded_vm.h"
#include "compiler.h"
#include "gc.h"

extern size_t trap_stack_off;
extern size_t s_off;

int main(void) {

    context_stack *cs = context_stack_init();
    gc_init();
    vm_init(cs);
    compiler_init();

    //bootstrapping the system
    {
        printf("Loading bootstrap.lisp.\n");
        FILE *f = fopen("/bootstrap.lisp", "r");
        if(!f) {
            printf("!!!!!!!!!!\nFailed to bootstrap this lisp. bootstrap.lisp is missing.\nGood luck.\n!!!!!!!!!!\n");
        }
        else {
            compiled_chunk *bootstrap = bootstrapper(cs, f);
            while(!feof(f)) {
                jmp_buf *jmper = vm_push_trap(cs, obj_nil());
                int ret = setjmp(*jmper);
                if(ret) {
                    if(feof(f)) {
                        break;
                    }
                    printf("CAUGHT AN ERROR: ");
                    print_object(pop());
                    printf("\nfailed to load bootstrapping file,\n");
                    dump_stack();
                    PANIC("Failed to load lisp bootstrap file.");
                }
                run_vm(cs, bootstrap);
                pop(); // pop result
                pop_trap();
            }
        }
    }

//    pthread_t gc_thread;
//    pthread_create(&gc_thread, NULL, run_gc_loop, cs);
    enable_gc = 1;

    printf("Starting REPL.\n");
    compiled_chunk *cc = repl(cs);
    while(1) {
        //printf("\n\n");
        jmp_buf *jmper = vm_push_trap(cs, obj_nil());
        int ret = setjmp(*jmper);
        //printf("RET IS: %d\n", ret);
        if(ret) {
            //printf("Continuing!\n");
            printf("CAUGHT AN ERROR: ");
            print_object(pop());
            printf("\n");
            if(feof(stdin)) {
                break;
            }
            continue;
        }
        pop(); // pop from (print ...)
//        printf("Dumping stack (%ld):\n", s_off);
//        dump_stack();
//        printf("CC: %p\n", cc);
//        printf("Current context: %p\nStack: %ld\nTrap: %ld",
//               top_context(cs), s_off, trap_stack_off);
        printf("\n> ");
        run_vm(cs, cc);
        pop_trap();
    }

    printf("Shutting down.\n");
    return 0;
}
