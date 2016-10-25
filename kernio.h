#ifndef KERNIO_H
#define KERNIO_H

#include "terminal.h"

/**
 * Supports %c %s %d and %x
 */
int printf(char *fmt, ...);
int sprintf(char *str, char *fmt, ...);

#endif
