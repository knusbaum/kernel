#include "common.h"
#include "kernio.h"
#include "terminal.h"
#include "vesa.h"
#include "kheap.h"
#include "errno.h"
#include "limits.h"

extern void halt();

void *memset(void *p, int c, size_t count)
{
    for(unsigned int i = 0; i < count; i++)
    {
        ((char *)p)[i] = c;
    }
    return p;
}

void *memcpy(void *dest, const void *src, size_t n) {

    char *cdest = dest;
    const char *csrc = src;

    size_t i;
    for(i = 0; i < n; i++) {
        cdest[i] = csrc[i];
    }

    return dest;
}

int k_toupper(int c) {
    if(c >= 97 && c <= 122) {
        return c - 32;
    }
    return c;
}
int k_tolower(int c) {
    if(c >=65 && c <= 90) {
        return c + 32;
    }
    return c;
}

char * itos(uint32_t myint, char buffer[], int bufflen)
{
    int i = bufflen - 2;
    buffer[bufflen-1] = 0;

    if(myint == 0) {
        buffer[i--] = '0';
    }

    while(myint > 0 && i >= 0)
    {
        buffer[i--] = (myint % 10) + '0';
        myint/=10;
    }

    return &buffer[i+1];
}

size_t strlen(const char* str) {
    size_t ret = 0;
    while ( str[ret] != 0 )
        ret++;
    return ret;
}

int strcmp(const char *s1, const char *s2) {
    while(*s1 && *s2) {
        if(*s1 != *s2) {
            return s1 - s2;
        }
        s1++;
        s2++;
    }
    if(*s1) return 1;
    if(*s2) return -1;
    return 0;
}

char *strdup(const char *s) {
    char *news = kmalloc(strlen(s) + 1);
    char *temp = news;
    while(*temp++ = *s++);
    return news;
}

char *strchr(const char *s, int c) {
    while(*s) {
        if(*s == c) return s;
        s++;
    }
    return NULL;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *begin;
    if(str) {
        begin = str;
    }
    else if (*saveptr) {
        begin = *saveptr;
    }
    else {
        return NULL;
    }

    while(strchr(delim, begin[0])) {
        begin++;
    }


    char *next = NULL;
    for(int i = 0; i < strlen(delim); i++) {
        char *temp = strchr(begin, delim[i]);
        if(temp < next || next == NULL) {
            next = temp;
        }
    }
    if(!next) {
        *saveptr = NULL;
        return begin;
    }
    *next = 0;
    *saveptr=next+1;
    return begin;
}

char *strcpy(char *dest, const char *src) {
    while(*dest++ = *src++);
}

char *strncpy(char *dest, const char *src, size_t n) {
    int i;
    while(i++ < n && (*dest++ = *src++));
}

int isdigit(int c) {
    return c > 47 && c < 58;
}

int isspace(int c) {
    return c == ' ' ||
        c == '\t' ||
        c == '\n' ||
        c == '\v' ||
        c == '\f' ||
        c == '\r';
}

long strtol(const char *restrict nptr, char **restrict endptr, int base) {
    const char *p = nptr, *endp;
    _Bool is_neg = 0, overflow = 0;
    /* Need unsigned so (-LONG_MIN) can fit in these: */
    unsigned long n = 0UL, cutoff;
    int cutlim;
    if (base < 0 || base == 1 || base > 36) {
//#ifdef EINVAL /* errno value defined by POSIX */
//        errno = EINVAL;
//#endif
        return 0L;
    }
    endp = nptr;
    while (isspace(*p))
        p++;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        is_neg = 1, p++;
    }
    if (*p == '0') {
        p++;
        /* For strtol(" 0xZ", &endptr, 16), endptr should point to 'x';
         * pointing to ' ' or '0' is non-compliant.
         * (Many implementations do this wrong.) */
        endp = p;
        if (base == 16 && (*p == 'X' || *p == 'x')) {
            p++;
        } else if (base == 0) {
            if (*p == 'X' || *p == 'x') {
                base = 16, p++;
            } else {
                base = 8;
            }
        }
    } else if (base == 0) {
        base = 10;
    }
    cutoff = (is_neg) ? -(LONG_MIN / base) : LONG_MAX / base;
    cutlim = (is_neg) ? -(LONG_MIN % base) : LONG_MAX % base;
    while (1) {
        int c;
        if (*p >= 'A')
            c = ((*p - 'A') & (~('a' ^ 'A'))) + 10;
        else if (*p <= '9')
            c = *p - '0';
        else
            break;
        if (c < 0 || c >= base) break;
        endp = ++p;
        if (overflow) {
            /* endptr should go forward and point to the non-digit character
             * (of the given base); required by ANSI standard. */
            if (endptr) continue;
            break;
        }
        if (n > cutoff || (n == cutoff && c > cutlim)) {
            overflow = 1; continue;
        }
        n = n * base + c;
    }
    if (endptr) *endptr = (char *)endp;
//    if (overflow) {
//        errno = ERANGE; return ((is_neg) ? LONG_MIN : LONG_MAX);
//    }
    return (long)((is_neg) ? -n : n);
}

int coerce_int(char *s, uint32_t *val) {
    uint32_t result = 0;

    while(*s && *s != '\n') {
        if(*s >= 48 && *s <= 57) {
            result *= 10;
            result = result + (*s - 48);
        }
        else {
            return 0;
        }
        s++;
    }
    *val = result;
    return 1;
}

uint8_t hex_char(uint8_t byte)
{
    byte = byte & 0x0F;
    if(byte < 0xA)
    {
        char buff[2];
        itos(byte, buff, 2);
        return buff[0];
    }
    else
    {
        switch(byte)
        {
        case 0x0A:
            return 'A';
            break;
        case 0x0B:
            return 'B';
            break;
        case 0x0C:
            return 'C';
            break;
        case 0x0D:
            return 'D';
            break;
        case 0x0E:
            return 'E';
            break;
        case 0x0F:
            return 'F';
            break;
        }
    }
    return 0;
}

void PANIC(char *err) {
    terminal_set_cursor(0, 1);
    set_vesa_color(make_vesa_color(0, 0, 0));
    set_vesa_background(make_vesa_color(255, 0, 0));
//    terminal_setcolor(make_color(COLOR_DARK_GREY, COLOR_BLACK));
//    terminal_settextcolor(make_color(COLOR_RED, COLOR_BLACK));
    printf("PANIC: %s", err);
    halt();
}
