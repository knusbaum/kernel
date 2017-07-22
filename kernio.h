#ifndef KERNIO_H
#define KERNIO_H

/**
 * Supports %c %s %d and %x
 */
int printf(char *fmt, ...);
int sprintf(char *str, char *fmt, ...);
void set_status(char *str);

#endif
