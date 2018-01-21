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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>
#include <util/delay.h>

#include "rtttl.hpp"
#include "avalue.hpp"
#include "servo.hpp"
#include "clock.hpp"
#include "7seg.hpp"
#include "button.hpp"

#define LOWBATTERYLEVEL 440 // low battery threshold in 10mV units
#define SERVINGSIZE 4 // size of one serving in sensor ticks
#define notRECHARGEABLE_BATTERY // if defined, enables trickle charging

#ifndef COUNTOF
#define COUNTOF(x) (int)(sizeof(x)/sizeof(x[0]))
#endif

#define spkr_on() (PORTB=PORTB & ~(_BV(PB1)))
#define spkr_off() (PORTB=PORTB|_BV(PB1))
#define spkr_toggle()  (PORTB=PORTB ^ _BV(PB1))

#define sensor_on() (PORTB=PORTB|_BV(PB7))
#define sensor_off() (PORTB=PORTB&~(_BV(PB7)))

// supply voltage meter initial constant
#define VCCCAL 799

typedef struct {
  uint8_t h, // hour
          m, // minute
          s; // servings
} FEEDINGTIME;

FEEDINGTIME EEMEM ee_feeding_schedule[10] = {
  { 7,00,6 },
  { 17,00,6 },
  { 22,30,6 },
  { 0,0,0 },
  { 0,0,0 },
  { 0,0,0 },
  { 0,0,0 },
  { 0,0,0 },
  { 0,0,0 },
  { 0,0,0 },
};

uint16_t EEMEM ee_calibration=VCCCAL;

//
FEEDINGTIME feeding_schedule[COUNTOF(ee_feeding_schedule)];
uint8_t feeding_date[10];
uint32_t scheduletimer;
Servo servo;
Clock clock;
Display display;
typedef enum { NONE,PLUS,MINUS,ENTER } BUTTON;
Button minus_button,plus_button,enter_button;
Avalue vcc;
RTTTL player;
enum { FULL, LOW, POWERSAVE };
int8_t powermode=FULL;
const char *menu[] = { "BAT", "CLK", "SCH","TST","CAL" };
enum { MENU_BAT,MENU_CLK,MENU_SCH,MENU_TST,MENU_CAL };
const char *edit_menu[] = { "HRS", "MIN", "SRV" };
enum { EDIT_HRS,EDIT_MIN,EDIT_SRV };
const char * select_menu[]={"F 1","F 2","F 3","F 4","F 5","F 6","F 7","F 8","F 9","F10"};
const char *clock_menu[] =  { "HRS","MIN","DAY","MON","YEA" };
enum { CLOCK_HRS,CLOCK_MIN,CLOCK_DAY,CLOCK_MON,CLOCK_YEA };
uint32_t menutimer;


void fullpower(void)
{
  powermode=FULL;
  set_sleep_mode(SLEEP_MODE_IDLE);
  display.On();
  spkr_off();
  sensor_on();
}

void lowpower(void)
{
  powermode=LOW;
  set_sleep_mode(SLEEP_MODE_IDLE);
  spkr_off();
  display.Off();
  sensor_off();
}

void powersave(void)
{
  powermode=POWERSAVE;
  display.Off();
  servo.Off();
  spkr_off();
  sensor_off();
  PORTC=6; // make sure pullups are enabled on plus and minus button
  PORTD=1; // enable pullup on enter button
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

BUTTON readbutton(void)
{
  if (minus_button.Read())
    return MINUS;
  if (plus_button.Read())
    return PLUS;
  if (enter_button.Read())
    return ENTER;
  return NONE;
}

void ServoWait()
{
  while (servo.Active()) {
    sleep_cpu();
    wdt_reset();
    WDTCSR|=0x40;
  }
}

// there is a 10K+68K voltage divider on VCC
// the ADC is measuring voltage across the 10K resistor
// calculating in millivolts this voltage is adcvalue*1100/1024
// the result then needs to be scaled up by the same ratio as
// voltage divider, the scaling factor is read from eeprom and
// can be adjusted through menu
int16_t read_battery_voltage(void)
{
uint32_t v;
  v=vcc.Get();
  v=(v*1100L)/1024L;
  v=(v*(int32_t)eeprom_read_word(&ee_calibration))/1000L;
  return (uint16_t)v;
}

void showbattery(void)
{
  while (clock.SecondsPassed(menutimer)<10) {
    display.printd(read_battery_voltage());
    if (readbutton()!=NONE)
      break;
    sleep_cpu();
    wdt_reset();
    WDTCSR|=0x40;
  }
}

// modify value in BCD form
uint16_t entervalue(uint16_t value,uint16_t min,uint16_t max)
{
  while (clock.SecondsPassed(menutimer)<10) {
    wdt_reset();
    WDTCSR|=0x40;
    display.printd(value);
    switch (readbutton()) {
      case PLUS:
        if (value<max)
          value++;
        menutimer=clock.ReadDayTime();
        break;
      case MINUS:
        if (value>min)
          value--;
        menutimer=clock.ReadDayTime();
        break;
      case ENTER:
        return value;
      default:
        break;
    }
  }
  return value;
}

bool StepBack()
{
uint8_t sensor=PINB&0x40;
  servo.Right(10);
  while (servo.Active()) {
    if ((PINB&0x40) && !sensor) {
      servo.Stop();
      return true;
    }
    sensor=PINB&0x40;
    wdt_reset();
    WDTCSR|=0x40;
  }
  servo.Stop();
  return false;
}

bool StepForward()
{
uint8_t sensor=PINB&0x40;
  servo.Left(10);
  while (servo.Active()) {
    if ((PINB&0x40) && !sensor) {
      servo.Stop();
      return true;
    }
    sensor=PINB&0x40;
    wdt_reset();
    WDTCSR|=0x40;
  }
  servo.Stop();
  return false;
}

void do_feeding(uint8_t servings,uint8_t playmusic=1)
{
  display.Clear();
  fullpower();
  if (playmusic)
    player.Play("Beethoven - Fur Elise : d=4,o=5,b=160:8e7,8d#7,8e7,8d#7,8e7,8b6,8d7,8c7,8a6,8e,8a,8c6,8e6,8a6,8b6,8e,8g#,8e6,8g#6,8b6,8c7,8e,8a,8e6,8e7,8d#7,8e7,8d#7,8e7,8b6,8d7,8c7,8a6,8e,8a,8c6,8e6,8a6,8b6,8e,8g#,8e6,8c7,8b6,2a6,");
  servings*=SERVINGSIZE;
  while (servings)
  {
    if (!StepForward())
    {
      servo.Off();
      _delay_ms(200);
      StepBack();
    }
    else
      servings--;
  }
  servo.Off();
  enter_button.Clear();
}

void clock_edit()
{
uint8_t h,m,s,Y,M,D,w;
int8_t item=0;
  menutimer=clock.ReadDayTime();
  while (clock.SecondsPassed(menutimer)<10) {
    wdt_reset();
    WDTCSR|=0x40;
    display.putc('\r');
    display.puts(clock_menu[item]);
    switch (readbutton()) {
      case PLUS:
        if (item<COUNTOF(clock_menu)-1)
          item++;
        menutimer=clock.ReadDayTime();
        break;
      case MINUS:
        if (item>0)
          item--;
        menutimer=clock.ReadDayTime();
        break;
      case ENTER:
        clock.ReadDateTime(Y,M,D,h,m,s,w);
        switch (item) {
          case CLOCK_HRS:
            h=entervalue(h,0,23);
            s=0;
            break;
          case CLOCK_MIN:
            m=entervalue(m,0,59);
            s=0;
            break;
          case CLOCK_DAY:
            D=entervalue(D,1,31);
            break;
          case CLOCK_MON:
            M=entervalue(M,1,12);
            break;
          case CLOCK_YEA:
            Y=entervalue(Y,0,99);
            break;
        }
        clock.ChangeDateTime(Y,M,D,h,m,s,w);        
        menutimer=clock.ReadDayTime();
        break;
      default:
        break;
    }
  }
}

void schedule_edit(uint8_t& h,uint8_t& m,uint8_t& servings)
{
int8_t item=0;
  menutimer=clock.ReadDayTime();
  while (clock.SecondsPassed(menutimer)<10) {
    wdt_reset();
    WDTCSR|=0x40;
    display.putc('\r');
    display.puts(edit_menu[item]);
    switch (readbutton()) {
      case PLUS:
        if (item<COUNTOF(edit_menu)-1)
          item++;
        menutimer=clock.ReadDayTime();
        break;
      case MINUS:
        if (item>0)
          item--;
        menutimer=clock.ReadDayTime();
        break;
      case ENTER:
        switch (item) {
          case EDIT_HRS: // hrs
            h=entervalue(h,0,23);
            break;
          case EDIT_MIN: // min
            m=entervalue(m,0,59);
            break;
          case EDIT_SRV: // lft
            servings=entervalue(servings,0,40);
            break;
        }
        menutimer=clock.ReadDayTime();
        break;
      default:
        break;
    }
  }
}

void schedule_select(void)
{
int8_t item=0;
  menutimer=clock.ReadDayTime();
  while (clock.SecondsPassed(menutimer)<10) {
    wdt_reset();
    WDTCSR|=0x40;
    display.putc('\r');
    display.puts(select_menu[item]);
    switch (readbutton()) {
      case PLUS:
        if (item<COUNTOF(select_menu)-1)
          item++;
        menutimer=clock.ReadDayTime();
        break;
      case MINUS:
        if (item>0)
          item--;
        menutimer=clock.ReadDayTime();
        break;
      case ENTER:
        schedule_edit(feeding_schedule[item].h,feeding_schedule[item].m,
          feeding_schedule[item].s);
        eeprom_write_byte(&ee_feeding_schedule[item].h,feeding_schedule[item].h);
        eeprom_write_byte(&ee_feeding_schedule[item].m,feeding_schedule[item].m);
        eeprom_write_byte(&ee_feeding_schedule[item].s,feeding_schedule[item].s);
        menutimer=clock.ReadDayTime();
        break;
      default:
        break;
    }
  }
}

void do_menu(void)
{
int8_t item=0;
uint16_t v;
  fullpower();
#ifdef RECHARGEABLE_BATTERY
  clock.EnableCharging();
#endif
  menutimer=clock.ReadDayTime();
  while (readbutton()==NONE); // it takes few timer ticks for button press to register
                              // wait for it, so that the wakeup press is ignored
  while (clock.SecondsPassed(menutimer)<10) {
    wdt_reset();
    WDTCSR|=0x40;
    display.putc('\r');
    display.puts(menu[item]);
    switch (readbutton()) {
      case PLUS:
        if (item<COUNTOF(menu)-1)
          item++;
        menutimer=clock.ReadDayTime();
        break;
      case MINUS:
        if (item>0)
          item--;
        menutimer=clock.ReadDayTime();
        break;
      case ENTER:
        switch (item) {
          case MENU_BAT:
            showbattery();
            break;
          case MENU_CLK:
            clock_edit();
            break;
          case MENU_SCH: // sch
            schedule_select();            
            break;
          case MENU_TST: // tst
            do_feeding(10,0);
            break;
          case MENU_CAL: // cal
            v=eeprom_read_word(&ee_calibration);
            v=entervalue(v,750,850);
            eeprom_write_word(&ee_calibration,v);
            break;
        }
        menutimer=clock.ReadDayTime();
        break;
      default:
        break;
    }
  }
}

// check if it is feeding time, return number
// of servings to deliver. 0 means no feeding time
//
uint8_t feeding_time(void)
{
uint8_t i,h,m,s,Y,M,D,w;
  clock.ReadDateTime(Y,M,D,h,m,s,w);
  for (i=0;i<COUNTOF(feeding_schedule);i++) {
    if (feeding_schedule[i].h==h && feeding_schedule[i].m==m && feeding_schedule[i].s) {
      if (feeding_date[i]!=D)
      {
        feeding_date[i]=D;
        return feeding_schedule[i].s;
      }
    }
  }
  return 0;
}

// when this is called we are in low power mode
// and when this function returns, the main loop
// will switch back to powersave mode
// any processing taking longer than second or two
// must reset watchdog periodically
//
void do_processing(void)
{
uint8_t h,m,s;
  // in powersave mode timers and everything else stops
  // so the first thing to do is to read the clock
  // to find out how long it has been
  clock.ReadTime(h,m,s);
#ifdef RECHARGEABLE_BATTERY
  static uint8_t cm=99;
  // charge clock battery for one watchdog period every minute
  if (m!=cm) {
    clock.EnableCharging();
    cm=m;
  }
  else {
    clock.DisableCharging();
  }
#endif
  // every 30 seconds check if it is feeding time
  if (clock.SecondsPassed(scheduletimer)<30)
    return;
  scheduletimer=clock.ReadDayTime();
  uint8_t servings=feeding_time();
  if (servings) {
    do_feeding(servings);
  }
  // check for low battery
  if (read_battery_voltage()<LOWBATTERYLEVEL) {
    // beep if low battery
    player.Play("beep:o=7,b=64: 32a7");
  }
}


ISR(TIMER0_OVF_vect)
{
static uint8_t servoticks;
uint16_t vv;
  // reset timer for next interrupt
  TCNT0=0xb0;
  servoticks++;
  if (powermode==FULL) {
    display.refresh();
    if (servoticks>7) {
      servo.Pulse();
    }
    // read buttons
    minus_button.Update((PINC & 0x02)>>1);
    plus_button.Update((PINC & 0x04)>>2);
    enter_button.Update(PIND & 1);
  }
  if (servoticks>7)
  {  
    // read supply voltage
    vv=ADCL;
    vv|=(ADCH<<8);
    ADCSRA=0xc3;  // start conversion again
    vcc.Update(vv);
    servoticks=0;
  }
}

// each watchdog interrupt shifts to low power mode
// for checking the time to see if anything needs to be done
//
ISR(WDT_vect)
{
  lowpower();  
}

ISR(PCINT1_vect)
{
  lowpower();
}

ISR(PCINT2_vect)
{
  lowpower();
}


/*
I/O configuration
-----------------
I/O pin                               direction    DDR  PORT

PC0 VCC/7                             ADC          0    0
PC1 MINUS button                      input        0    1
PC2 PLUS button                       input        0    1
PC3 clock CLK                         output       1    0
PC4 clock I/O                         output       1    0
PC5 clock /RST                        output       1    0

PD0 ENTER button                      input        0    1
PD1 A segment                         output       1    0
PD2 B segment                         output       1    0
PD3 servo PWM                         output       1    0
PD4 servo power                       output       1    0
PD5 C segment                         output       1    0
PD6 D segment                         output       1    0
PD7 E segment                         output       1    0

PB0 F segment                         output       1    0
PB1 G segment                         output       1    0
PB2 LED                               output       1    0
PB3 D3 cathode                        output       1    0
PB4 D2 cathode                        output       1    0
PB5 D1 cathode                        output       1    0
PB6 movement sensor input             input        0    0
PB7 movement sensor power             output       1    0
*/

int main(void)
{
  MCUSR=0;
  MCUCR=0;
  // I/O directions
  DDRC=0x38;
  DDRD=0xfe;
  DDRB=0xbf;
  // initial state
  PORTC=0x06;
  PORTD=0x01;
  PORTB=0x00;
  //
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  //
  PCMSK2=0x01; // mask out everything but button pins
  PCMSK1=0x06;
  // configure watchdog
  WDTCSR=(1<<WDE) | (1<<WDCE);
  WDTCSR=(1<<WDE) | (1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0) ; // 2sec timout, interrupt+reset
  // configure timer0 for periodic interrupts
  TCCR0B=4; // timer0 clock prescaler to 256
  TIMSK0=1; // enable overflow interrupts
  //
  DIDR0=1;
  ADMUX=0xc0;  // channel 0, internal 1.1V reference
  ADCSRA=0xc3; // interrupts disabled, start conversion,  prescaler 8
  while (ADCSRA&0x40) // wait until conversion completes
    ;

  fullpower();
  // copy feeding schedule to RAM for faster access
  // to conserve power
  eeprom_read_block (&feeding_schedule,&ee_feeding_schedule,sizeof(feeding_schedule));
  //
  clock.EnsureRunning();
#ifndef RECHARGEABLE_BATTERY
  clock.DisableCharging();
#endif
  sei();
  scheduletimer=clock.ReadDayTime();
  while (1) {
    PCICR=0x06; // enable pin change interrupts 1 and 2
    sleep_cpu();   // watchdog or I/O interrupt wakes us up
    wdt_reset();
    WDTCSR=(1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0) ; // 2sec timout, interrupt+reset
    PCICR=0x00;
    // now check if it was button press or watchdog
    if ((PIND&1)==0 || (PINC&0x06)!=0x06) {
      do_menu();
    }
    else {
      do_processing();
    }
    powersave();   // preapare to take another nap
  }
}

