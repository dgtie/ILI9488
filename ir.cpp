/*  RC5 - OC1
 *  RA9 - IC3: IFS0, IEC0, IPC3
 *  TIMER1: IFS0, IEC0, IPC1 (bit stream)
 *  TIMER2: IFS0, IEC0, IPC2 (carrier)
 *  TIMER3 is default IC3 clock
 *  PBCLK = 40 MHz
 */

#include <xc.h>

bool ir_repeat(void);

namespace
{

unsigned char *ptr;
unsigned short bits, count;

void ir_off(void) {
  OC1CONbits.ON = 0;
  T1CONbits.ON = 0;
  T2CONbits.ON = 0;
}

void next(void) {
  if (*ptr == (unsigned char)0x80) {
    if (ir_repeat()) ptr -= ptr[1];
    else return ir_off();
  }
  if (!(bits = *ptr++)) ir_off();
  else {
    if (bits & 1) {
      count = (bits & 0x7f) >> 1;
      count--;
      bits &= 0x80;
    }
  }
}

} // anonymous

void ir_init(void) {
  IPC1bits.T1IP = 1;
  IFS0CLR = _IFS0_T1IF_MASK;
  IEC0bits.T1IE = 1; 
//  PR2 = 999;			// 40 kHz (1000-40, 1052-38, 1111-36)
  OC1CONbits.OCM = 0b110;	// PWM mode, Fault pin disabled
  OC1R = OC1RS = 250;
  T3CONbits.TCKPS = 0b111;	// 40M/256 = 6.4 us
  IPC3bits.IC3IP = 1;
  IC3CONbits.ICM = 1;		// Edge Detect mode - rising and falling
  IC3CONbits.ON = 1;
}

void ir_send(unsigned char *p) {
  if (T1CONbits.ON) return;
  bits = count = 0;
  unsigned c = 1000 + *p++;
  unsigned d = c * *p++;
  PR1 = --d;
  PR2 = --c;
  ptr = p;
  IFS0CLR = _IFS0_T1IF_MASK;
  T2CONbits.ON = 1; 
  T1CONbits.ON = 1; 
}

extern "C"
void __attribute__((interrupt(ipl1soft), vector(_TIMER_1_VECTOR), nomips16))
t1(void) {
  IFS0CLR = _IFS0_T1IF_MASK;
  if (T1CONbits.ON) {
    if (bits & 0x80) OC1CONbits.ON = 1;
    else OC1CONbits.ON = 0;
    if (count) count--;
    else {
      if (bits & 0x3f) bits <<= 1;
      else next();
    }
  }
}
