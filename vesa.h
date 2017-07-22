#ifndef VESA_H
#define VESA_H

void set_vmode();

uint32_t make_vesa_color(uint8_t r, uint8_t g, uint8_t b);

void draw_pixel_at(int x, int y, uint32_t color);

#endif
