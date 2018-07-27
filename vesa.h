#ifndef VESA_H
#define VESA_H

void set_vmode();

uint32_t make_vesa_color(uint8_t r, uint8_t g, uint8_t b);

void draw_pixel_at(int x, int y, uint32_t color);

void draw_character_at(int x, int y, int c, uint32_t fg, uint32_t bg);

uint32_t get_framebuffer_addr();
uint32_t get_framebuffer_length();

void set_vesa_color(uint32_t color);
void set_vesa_background(uint32_t color);

uint32_t get_vesa_color();
uint32_t get_vesa_background();

#endif
