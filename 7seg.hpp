/*
MIT License

Copyright (c) 2017 Madis Kaal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __7seg_hpp__
#define __7seg_hpp__

#include <avr/io.h>

#ifndef COUNTOF
#define COUNTOF(x) (int)(sizeof(x)/sizeof(x[0]))
#endif

struct _font
{
  char c;
  uint8_t bits;
};

// this character to bit pattern map gets simply linear searched
// performance is not that important, so optimizing for size instead
static struct _font font[] =
{
  { ' ',  0x00 },
  { '-',  0x40 },
  { '0',  0x3f },
  { '1',  0x06 },
  { '2',  0x5b },
  { '3',  0x4f },
  { '4',  0x66 },
  { '5',  0x6d },
  { '6',  0x7d },
  { '7',  0x07 },
  { '8',  0x7f },
  { '9',  0x6f },
  { 'A',  0x77 },
  { 'C',  0x39 },
  { 'E',  0x79 },
  { 'F',  0x71 },
  { 'G',  0x3d },
  { 'K',  0x76 },
  { 'L',  0x38 },
  { 'M',  0x37 },
  { 'P',  0x73 },
  { 'S',  0x6d },
  { 'W',  0x3e },
  { 'X',  0x76 },
  { 'B',  0x7c },
  { 'C',  0x58 },
  { 'D',  0x5e },
  { 'H',  0x74 },
  { 'I',  0x04 },
  { 'J',  0x04 },
  { 'N',  0x54 },
  { 'O',  0x5c },
  { 'Q',  0x5c },
  { 'R',  0x50 },
  { 'T',  0x78 },
  { 'U',  0x1c },
  { 'V',  0x1c },
  { 'Y',  0x6e },
  { 'Z',  0x5b }
};

class Display
{
  uint8_t chars[3];
  uint8_t idx,dp;
  uint8_t off;
    
public:
  Display()
  {
    chars[0]=0;
    chars[1]=0;
    chars[2]=0;
    idx=0;
    dp=0;
    off=0;
  }
  
  void On() { off=0; }
  void Off() { off=1; PORTB|=0x38; }

  void Clear() { 
    chars[0]=0x00; chars[1]=0x00; chars[2]=0x00;
    idx=0; dp=0; off=0;
  }
    
  void refresh(void)
  {
    PORTB|=0x38; // all digits off
    if (off)
      return;
    uint8_t bits=chars[dp];
    PORTB&=0x3a; // all segments
    PORTD&=0x19; // off too
    PORTB|=(((bits>>5)&1) | ((bits>>4)&4));
    PORTD|=((bits<<1)&6) | ((bits<<3)&0xe0);
    switch (dp) {
      case 0:
        PORTB&=0x1f;
        break;
      case 1:
        PORTB&=0x2f;
        break;
      case 2:
        PORTB&=0x37;
        break;
    }
    dp=(dp+1)%3;
  }
  
  void putc(uint8_t c)
  {
    uint8_t i;
    switch (c)
    {
      case '\r':
        idx=0;
        return;
      case '\n':
        chars[0]=0;
        chars[1]=0;
        chars[2]=0;
        idx=0;
        return;
      default:
        for (i=0;i<COUNTOF(font);i++) {
          if (font[i].c==c) {
            chars[idx++]=font[i].bits;
            idx%=3;
            return;
          }
        }
        chars[idx++]=0;
        idx%=3;
        return;
    }
  }

  void puts(const char *s)
  {
    while (s && *s)
      putc(*s++);
  }  
  
  void putx(uint8_t c)
  {
    c=(c&0x0f)+'0';
    if (c>'9')
      c+=7;
    putc(c);
  }

  void printx(uint16_t v)
  {
    idx=0;
    putx(v>>8);
    putx(v>>4);
    putx(v);
  }
  
  void printd(uint16_t v)
  {  
    putc('\r');
    putc((v%1000)/100+'0');
    putc((v%100)/10+'0');
    putc((v%10)+'0');
  }
  
};

#endif
