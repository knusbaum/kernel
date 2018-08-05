#include "common.h"
#include "kernio.h"
#include "terminal.h"
#include "vesa.h"
#include "kheap.h"

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
