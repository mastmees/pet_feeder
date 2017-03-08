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
#ifndef __servo_hpp__
#define __servo_hpp__

#include <avr/io.h>

#define servo_power_off() (PORTD&=(~_BV(PD4)))
#define servo_power_on() (PORTD |= _BV(PD4))

// Pulse() needs to be called every 20ms to run the servo
//
class Servo
{
volatile uint16_t pcount;
volatile uint8_t active;

public:
  Servo()
  {
    TCCR2B=0;    // stop counter by disconnecting clock
    TCNT2=0;     // start counting at bottom
    OCR2A=0;     // set compare value to 0, this is where the counter resets
    TCCR2A=0x33; // fast PWM mode 7, output on OC2B, set at compare match
                 // clear at bottom
    TCCR2B=0x0c;
    OCR2B=255-120-1;
    On();
  }

  uint8_t GetPosition(void)
  {
    return 255-OCR2B-1;
  }
  
  void SetPosition(uint8_t pos)
  {
    On();
    OCR2B=255-pos-1;
  }

  void Left(uint16_t pulses)
  {
    SetPosition(120);
    pcount=pulses;
  }

  void Right(uint16_t pulses)
  {
    SetPosition(250);
    pcount=pulses;
  }
  
  void On()
  {
    pcount=0;
    active=1;
    servo_power_on();
  }
  
  void Off()
  {
    servo_power_off();
    active=0;
  }
  
  void Stop()
  {
    pcount=0;
    active=0;
    TCNT2=OCR2B-1;
  }

  bool Active()
  {
    return (active>0);
  }

  // -1 left
  // 0 stopped
  // 1 right
  int8_t Direction()
  {
    if (!Active())
      return 0;
    if (GetPosition()>180)
      return 1;
    return -1;
  }
      
  void Pulse()
  {
    if (!pcount) {
      active=0;
    }
    else
    {
      pcount--;
      TCNT2=OCR2B-1;
    }
  }
  
};

#endif
