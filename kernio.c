#include "kernio.h"
#include "terminal.h"
#include "common.h"
#include <stdarg.h>

int BOOTED;

// Potential base for stdarg. Doesn't work.
// We need to know more about how gcc handles the stack.
//typedef char *va_list;
//#define va_start(ap,parmn) (void)((ap) = (char*)(&(parmn) + 1))
//#define va_end(ap) (void)((ap) = 0)
//#define va_arg(ap, type)                          \
//    (((type*)((ap) = ((ap) + sizeof(type))))[-1])

int printf(char *fmt, ...) {
    //return;
    va_list argp;
    va_start(argp, fmt);

    char *p;
    for(p = fmt; *p != 0; p++) {
        if(*p != '%') {
            terminal_putchar(*p);
            continue;
        }
        p++; // Skip the %
        int i;
        char *s;
        switch(*p) {
        case 'c':
            i = va_arg(argp, int);
            terminal_putchar(i);
            break;
        case 's':
            s = va_arg(argp, char *);
            terminal_writestring(s);
            break;
        case 'd':
            i = va_arg(argp, int);
            terminal_write_dec(i);
            break;
        case 'p':
        case 'x':
            i = va_arg(argp, int);
            terminal_write_hex(i);
            break;
        case '%':
            terminal_putchar('%');
            break;
        case 'l':
            if(p[1] == 'd') {
                p++;
                long int j = va_arg(argp, long int);
                terminal_write_dec(j);
            }
            break;
            
        default:
            terminal_putchar('%');
            terminal_putchar(*p);
            printf("UNKNOWN SPECIAL CHARACTER: %c", *p);
            PANIC("UNKNOWN SPECIAL CHARACTER");
            break;
        }
    }
    return 0;
}

int sprintf(char *str, char *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);

    char *p;
    for(p = fmt; *p != 0; p++) {
        if(*p != '%') {
            *str++ = *p;
            continue;
        }
        p++; // Skip the %
        int i;
        char *s;
        switch(*p) {
        case 'c':
            i = va_arg(argp, int);
            *str++ = i;
            break;
        case 's':
            s = va_arg(argp, char *);
            while(*s) {
                *str++ = *s++;
            }
            break;
        case 'd':
            i = va_arg(argp, int);
            char decbuff[13]; // At most 12 decimal places for 32 bit int.
            char *dec = itos(i, decbuff, 13);
            while(*dec) {
                *str++ = *dec++;
            }
            break;
        case 'p':
        case 'x':
            i = va_arg(argp, int);
            for(int j = 28; j >= 0; j-=4)
            {
                *str++ = hex_char(i>>j);
            }
            break;
        case '%':
            *str++ = '%';
            break;
        case 'l':
            if(p[1] == 'd') {
                p++;
                long int j = va_arg(argp, long int);
                char decbuff[19]; // At most 12 decimal places for 32 bit int.
                char *dec = itos(j, decbuff, 19);
                while(*dec) {
                    *str++ = *dec++;
                }
            }
            break;
        default:
            *str++ = '%';
            *str++ = *p;
            printf("UNKNOWN SPECIAL CHARACTER: %c", *p);
            PANIC("UNKNOWN SPECIAL CHARACTER");
            break;
        }
    }
    *str++ = 0;
    return 0;
}

void set_status(char *str) {
    if(terminal_set_status) {
        terminal_set_status(str);
    }
}
