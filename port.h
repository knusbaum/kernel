#ifndef PORT_H
#define PORT_H
#include "stdint.h"

extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern void insw(uint16_t port, void *addr, unsigned int count);
extern void outsw(uint16_t port, void *addr, unsigned int count);

#endif
