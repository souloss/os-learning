#define VGA_ADDR ((volatile unsigned short *)0xC00B8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define WHITE_ON_BLACK 0x0F

void print(const char *s)
{
  volatile unsigned short *vga = VGA_ADDR;
  while (*s)
  {
    *vga++ = (WHITE_ON_BLACK << 8) | *s++;
  }
}

void clear_screen()
{
  volatile unsigned short *vga = VGA_ADDR;
  unsigned short blank = (WHITE_ON_BLACK << 8) | ' ';

  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    vga[i] = blank;
}

void main()
{
  __asm__ volatile("movl $0x1, %eax");
  clear_screen();
  print("Hello OS");
  while (1)
    ;
}