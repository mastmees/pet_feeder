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
#ifndef __clock_hpp__
#define __clock_hpp__
#include <avr/io.h>

class Clock
{

  #define clk_low() (PORTC &= (~_BV(PC3)))
  #define clk_high() (PORTC |= _BV(PC3))
  #define _rst_high() (PORTC |= _BV(PC5))
  #define _rst_low() ( PORTC &= (~_BV(PC5)))
  #define io_high() (PORTC |= _BV(PC4))
  #define io_low() (PORTC &= (~_BV(PC4)))
  #define io_bit() (PINC & _BV(PC4))
  #define io_input() ( DDRC &= (~_BV(PC4)))
  #define io_output() ( DDRC |= _BV(PC4))
    
  // clock out one data byte, low bit first
  // clock signal left high at last bit to allow
  // direction reversal
  void send(uint8_t c)
  {
    uint8_t i;
    for (i=0;i<8;i++) {
      if (c&1)
        io_high();
      else
        io_low();
      clk_high();
      c=c>>1;
      if (i<7)
        clk_low();
    }
  }

  // clock in one data byte
  uint8_t recv(void)
  {
    uint8_t v=0,i;
    for (i=0;i<8;i++)
    {
      v>>=1;
      if (io_bit())
        v|=0x80;
      clk_high();
      clk_low();
    }
    return v;
  }

  // clock low, rst low - stop condition
  void rst_low(void)
  {
     clk_low();
     _rst_low();
  }

  // clock low, rst high - start condition
  void rst_high(void)
  {
    rst_low();
    clk_low();
    io_output();
    io_low();
    _rst_high();
  }

  uint8_t read(uint8_t adr)
  {
    rst_high();
    send(adr);
    io_input();
    clk_low();
    adr=recv();
    rst_low();
    io_output();
    return adr;
  }

  void write(uint8_t adr,uint8_t d)
  {
    rst_high();
    send(adr);
    clk_low();
    send(d);
    clk_low();
    rst_low();
  }

  uint8_t tobin(uint8_t bcd)
  {
    return ((bcd>>4)*10)+(bcd&15);
  }

  uint8_t tobcd(uint8_t bin)
  {
    return ((bin/10)<<4)+(bin%10);
  }

public:

  void EnableCharging()
  {
    write(0x8e,0); // enable writing
    write(0x90,0xa5); // set trickle charge to 1 diode, 2kohm resistor
    write(0x8e,0x80); // disable writing
  }

  void DisableCharging()
  {
    write(0x8e,0);
    write(0x90,0);
    write(0x8e,0x80);
  }
    
  void ChangeDateTime(uint8_t Y,uint8_t M,uint8_t D,uint8_t h,uint8_t m,uint8_t s,uint8_t w)
  {
    write(0x8e,0);
    write(0x80,tobcd(s));
    write(0x82,tobcd(m));
    write(0x84,tobcd(h)); // 24H format
    write(0x86,tobcd(D));
    write(0x88,tobcd(M));
    write(0x8a,tobcd(w));
    write(0x8c,tobcd(Y));
    write(0x8e,0x80);
  }

  Clock()
  {
  }

  void EnsureRunning()
  {
    if (read(0x81)&0x80) {             // see of clock halt is set
      ChangeDateTime(16,7,10,0,0,0,7); // reset time to a sensible value    
    }
  }

  void SetHour(uint8_t h)
  {
    write(0x8e,0);
    write(0x84,tobcd(h));
    write(0x8e,0x80);
  }

  void SetMinute(uint8_t m)
  {
    write(0x8e,0);
    write(0x82,tobcd(m));
    write(0x8e,0x80);
  }

  // all values are BCD numbers  
  void ReadDateTime(uint8_t& Y,uint8_t& M,uint8_t& D,uint8_t& h,uint8_t& m,uint8_t& s,uint8_t& w)
  {
    s=tobin(read(0x81)&0x7f);
    m=tobin(read(0x83));
    h=tobin(read(0x85)&0x3f);
    D=tobin(read(0x87));
    M=tobin(read(0x89));
    w=tobin(read(0x8b));
    Y=tobin(read(0x8d));
  }

  void ReadTime(uint8_t& h,uint8_t& m,uint8_t& s)
  {
    m=tobin(read(0x83));
    h=tobin(read(0x85)&0x3f);
    s=tobin(read(0x81)&0x7f);
  }
    
  // returns number of current second within current day
  int32_t ReadDayTime(void)
  {
    uint8_t h,m,s;
    s=tobin(read(0x81)&0x7f);
    m=tobin(read(0x83));
    h=tobin(read(0x85)&0x3f);
    return (h*60L*60L)+(m*60L)+s;
  }

  // returns number of seconds passed since the daytime value
  uint32_t SecondsPassed(int32_t fromtime)
  {
    int32_t t=ReadDayTime();
    if (t>=fromtime)
      return t-fromtime;
    // if current is smaller, then the next day has arrived
    // and the number of seconds passed is current time plus
    // the number of seconds that were left till end of day
    return t+((24L*60L*60L)-fromtime);
  }
};
#endif
