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
#ifndef __rtttl_hpp__
#define __rtttl_hpp__
#include <ctype.h>

/*
Sample RTTTL-format ringtone (Imperial theme from Star Wars):

Imperial:d=4, o=5, b=100:e, e, e, 8c, 16p, 16g, e, 8c, 16p, 16g, e, p, b, b, b, 8c6, 16p, 16g, d#, 8c, 16p, 16g, e, 8p

Official Specification

<ringing-tones-text-transfer-language> :=
   <name> <sep> [<defaults>] <sep> <note-command>+
<name> := <char>+ ; maximum name length 10 characters
<sep> := ":"
<defaults> := 
   <def-note-duration> |
   <def-note-scale> |
   <def-beats> 
<def-note-duration> := "d=" <duration>
<def-note-scale> := "o=" <scale> 
<def-beats> := "b=" <beats-per-minute>
<beats-per-minute> := 25,28,...,900 ; decimal value
; If not specified, defaults are
   ;
   ; 4 = duration
   ; 6 = scale 
   ; 63 = beats-per-minute
   <note-command> :=
   [<duration>] <note> [<scale>] [<special-duration>] <delimiter>
   <duration> :=
   "1" | ; Full 1/1 note
   "2" | ; 1/2 note
   "4" | ; 1/4 note
   "8" | ; 1/8 note
   "16" | ; 1/16 note
   "32" | ; 1/32 note
   
   <note> :=
   "P"  | ; pause
   "C"  |
   "C#" |
   "D"  |
   "D#" |
   "E"  |
   "F"  |
   "F#" |
   "G"  |
   "G#" |
   "A"  |
   "A#" |
   "H" 
<scale> :=
   "5" | ; Note A is 440Hz
   "6" | ; Note A is 880Hz
   "7" | ; Note A is 1.76 kHz
   "8" ; Note A is 3.52 kHz
<special-duration> :=
   "." ; Dotted note
<delimiter> := ","
; End of specification
*/

struct note {
  uint8_t name;  // high bit set for sharp
  uint16_t freq; // rounded to closest full Hz
};

static const struct note notes[4][12] = {
  // scale 5
  {
    {'C',262 },
    {'C'|0x80,277},
    {'D',294},
    {'D'|0x80,311},
    {'E',330},
    {'F',349},
    {'F'|0x80,370},
    {'G',392},
    {'G'|0x80,415},
    {'A',440},
    {'A'|0x80,466},
    {'B',494}
  },
  // scale 6
  {
    {'C',523},
    {'C'|0x80,554},
    {'D',587},
    {'D'|0x80,622},
    {'E',659},
    {'F',698},
    {'F'|0x80,740},
    {'G',784},
    {'G'|0x80,831},
    {'A',880},
    {'A'|0x80,932},
    {'B',988}
  },
  // scale 7
  { 
    {'C',1047},
    {'C'|0x80,1109},
    {'D',1175},
    {'D'|0x80,1245},
    {'E',1319},
    {'F',1397},
    {'F'|0x80,1480},
    {'G',1568},
    {'G'|0x80,1661},
    {'A',1760},
    {'A'|0x80,1865},
    {'B',1976}
  },
  // scale 8
  {
    {'C',2093},
    {'C'|0x80,2217},
    {'D',2349},
    {'D'|0x80,2489},
    {'E',2637},
    {'F',2794},
    {'F'|0x80,2960},
    {'G',3136},
    {'G'|0x80,3322},
    {'A',3520},
    {'A'|0x80,3729},
    {'B',3951}
  }
};
 
class RTTTL
{
  uint16_t getvalue(const char *& score)
  {
    uint16_t v=0;
    while (score && (isdigit(*score) || *score=='=' || *score==' '))
    {
      if (isdigit(*score))
        v=v*10+(*score-'0');
      score++; 
    }
    return v;
  }

  uint16_t notefrequency(uint8_t nn,uint16_t scale)
  {
    uint8_t j;
    if (nn=='P' or (scale<5 || scale>8))
      return 0;
    scale-=5;
    for (j=0;j<sizeof(notes[0])/sizeof(notes[0][0]);j++) {
      if (notes[scale][j].name==nn)
        return notes[scale][j].freq;
    }
    return 0;
  }
    
public:
  RTTTL()
  {
  }

  // speaker is wired between VCC and oc1a, in series with resistor
  //
  void Tone(uint16_t freq,uint16_t length)
  {
    TCCR1B=0;      // stop clock
    if (freq) {
      freq=((F_CPU/(uint32_t)freq)/2)-1;
      OCR1A=freq;  // set the top value, this defines the frequency
      TCNT1=0;     // reset counter to make sure 1st count is correct
      TCCR1A=0x43; // mode 15, toggle OC1A on compare match
      TCCR1B=0x19; // mode 15, f/8 prescaling
    }
    while (length--)
      _delay_ms(1);
    TCCR1B=0;    // stop clock
    OCR1A=0;
    TCCR1A=0;
    (PORTB=PORTB|_BV(PB1)); // make output high so that current does not flow
    wdt_reset();
    WDTCSR|=0x40;
  }

  // play entire RTTTL score
  void Play(const char *score)
  {
    uint16_t duration=4,scale=6,bpm=63,nd=0,ns=0;
    uint8_t nn;
    if (!score)
      return;
    while (*score && *score!=':')
      score++; // skip the name
    if (*score==':')
      score++;   // and separator
    else
      return;
    // parse defaults section now
    while (*score && *score!=':') {
      // ignore spaces and plain separators
      if (*score==' ' || *score==',') {
        score++;
        continue;
      }
      switch (*score) {
        case 'd':
          duration=getvalue(++score);
          break;          
        case 'o':
          scale=getvalue(++score);
          break;          
        case 'b':
          bpm=getvalue(++score);
          break;
        default: // invalid character, skip to separator or delimiter
          while (*score && *score!=',' && *score!=':')
            score++;
          break;
      }
    }
    // parse and play notes
    while (*score) {
      if (isdigit(*score))
        nd=getvalue(score);
      else
        nd=duration;
      if (!*score)
        return;
      nn=toupper(*score++);
      if (!*score)
        return;
      if (*score=='#') {
        nn|=0x80;
        score++;
      }
      if (*score=='.') { // by spec special duration should only come at the end
        nd=nd*4/3;       // but in practice it is sometimes stuck in the middle
        score++;
      }
      // get scale if present
      if (isdigit(*score))
        ns=getvalue(score);
      else
        ns=scale;
      if (*score=='.') { // 1.5 times duration
        nd=nd*4/3;
        score++;
      }
      while (*score && (*score==',' || *score==' '))
        score++;  // skip trailing separators
      nd=(60000/bpm)*4/nd;  // convert note duration to ms
      Tone(notefrequency(nn,ns),nd);
    }
  }
};
#endif
