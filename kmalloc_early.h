#ifndef KMALLOC_EARLY_H
#define KMALLOC_EARLY_H

#include "stdint.h"
#include "stddef.h"

uint32_t e_kmalloc_a(uint32_t sz);                  // page aligned.
uint32_t e_kmalloc_p(uint32_t sz, uint32_t *phys);  // returns a physical address.
uint32_t e_kmalloc_ap(uint32_t sz, uint32_t *phys); // page aligned and returns a physical address.
uint32_t e_kmalloc(uint32_t sz);                    // vanilla (normal).

// Returns a pointer to the first usable page after the kernel and early-allocated data.
// After this, any kmalloc calls will cause a panic.
uint32_t disable_early_kmalloc();

#endif
