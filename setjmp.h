#ifndef SETJMP_H
#define SETJMP_H

typedef struct {
    unsigned long jb[6];
    unsigned long fl;
    unsigned long ss[128/sizeof(long)];
} jmp_buf[1];


int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#endif
