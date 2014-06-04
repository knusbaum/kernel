#include <stddef.h>
#include <stdint.h>
#include "terminal.h"
#include "port.h"

#define VGA_COMMAND_PORT 0x3D4
#define VGA_DATA_PORT    0x3D5
#define VGA_CURSOR_HIGH  14
#define VGA_CURSOR_LOW   15

uint8_t make_color(enum vga_color fg, enum vga_color bg)
{
  return fg | bg << 4;
}
 
uint16_t make_vgaentry(char c, uint8_t color)
{
  uint16_t c16 = c;
  uint16_t color16 = color;
  return c16 | color16 << 8;
}
 
size_t strlen(const char* str)
{
  size_t ret = 0;
  while ( str[ret] != 0 )
    ret++;
  return ret;
}

char * itos(uint32_t myint, char buffer[], int bufflen)
{
  int i = bufflen - 2;
  buffer[bufflen-1] = 0;
  
  while(myint > 0 && i >= 0)
    {
      buffer[i--] = (myint % 10) + '0';
      myint/=10;
    }
  
  return &buffer[i+1];
}
 
void terminal_initialize()
{
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
  terminal_buffer = (uint16_t*) 0xB8000;
  for ( size_t y = 0; y < VGA_HEIGHT; y++ )
    {
      for ( size_t x = 0; x < VGA_WIDTH; x++ )
	{
	  const size_t index = y * VGA_WIDTH + x;
	  terminal_buffer[index] = make_vgaentry(' ', terminal_color);
	}
    }
}
 
void terminal_setcolor(uint8_t color)
{
  terminal_color = color;
}
 
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = make_vgaentry(c, color);
}

static void move_cursor(uint8_t xpos, uint8_t ypos)
{
  uint16_t location = ypos * VGA_WIDTH + xpos;

  outb(VGA_COMMAND_PORT, VGA_CURSOR_HIGH); // We're setting the high cursor byte
  outb(VGA_DATA_PORT, location >> 8);     
  outb(VGA_COMMAND_PORT, VGA_CURSOR_LOW); // We're setting the low cursor byte
  outb(VGA_DATA_PORT, location);
}

void terminal_scroll()
{
  uint16_t blank = make_vgaentry(' ', terminal_color);
  //Move the lines up
  for(unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
      terminal_buffer[i] = terminal_buffer[i+VGA_WIDTH];
    }

  //Clear the last line
  for(unsigned int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++)
    {
      terminal_buffer[i] = blank;
      terminal_column = 0;
    }
}

void terminal_advance()
{
  if ( ++terminal_column == VGA_WIDTH)
    {
      terminal_newline();
    }
  else
    {
      //      move_cursor(terminal_column, terminal_row);
    }
}

void terminal_newline()
{
  terminal_column = 0;
  if (++terminal_row == VGA_HEIGHT)
    {
      //      terminal_scroll();
      // terminal_row--;
      terminal_row = 0;
    }
  //  move_cursor(terminal_column, terminal_row);
}
 
void terminal_putchar(char c)
{
  switch (c)
    {
    case '\n':
      terminal_newline();
      break;
    default:
      terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
      terminal_advance();
      break;
    }
}
 
void terminal_writestring(const char* data)
{
  size_t datalen = strlen(data);
  for ( size_t i = 0; i < datalen; i++ )
    {
      terminal_putchar(data[i]);
    }
}

void terminal_write_dec(uint32_t d)
{
  char buff[13];
  char *x = itos(d, buff, 13);
  terminal_writestring(x);
}
