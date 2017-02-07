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
#ifndef __avalue_hpp__
#define __avalue_hpp__

#include <avr/io.h>

// this is a very simple analog value accumulator
// the internal value is updated every 64 updates with
// an average value of these 64
class Avalue
{
  uint16_t v,av,c;
public:
  Avalue()
  {
    v=0; av=0; c=0;
  }
  
  void Clear(void)
  {
    v=0; av=0; c=0;
  }
  
  void Update(uint16_t a)
  {
    av+=a;
    c++;
    if (c==64) {
      v=av/64;
      c=0;
      av=0;
    }
  }
  
  uint16_t Get(void)
  {
    return v;
  }

};
#endif
