#include <xc.h>

/*  ILI9488
 *  *******
 *  SCK1 - RB14 (IFS1, IEC1, IPC7)
 *  CSX  - RC7
 *  MOSI - RB13 (SDO1)
 *  MISO - RA1 (SDI1)
 *  RESET - RA10
 *  D/CX - RA7
 *
 *  BL - RA0
 *
 *  other CS: RB0, RC7, RC4
 */

/*
 *  SCK2 - RB15 (IFS1, IEC1, IPC9)
 *  CSX  - RA0
 *  SDO2 - RB1
 *  SDI2 - RB2
 *  INT4 - RB3 (IFS0, IEC0, IPC4)
 */

/*  SD CARD
 *  *******
 *  SCK2 - RB15 (IFS1, IEC1, IPC9)
 *  CSX  - RC4
 *  MOSI - RB5
 *  MISO - RC3
 *
 *  other CS: RB0, RC7
 */

/*  RC9 - OC3 (buzzer)
 *
 */

/*  RC5 - OC1
 *  RA9 - IC3: IFS0, IEC0, IPC3
 *  TIMER1: IFS0, IEC0, IPC1 (bit stream)
 *  TIMER2: IFS0, IEC0, IPC2 (carrier)
 *  TIMER3 is default IC3 clock
 *  PBCLK = 40 MHz
 */

#define MS 20000        // 1 ms

int read_cdc(char*&);
bool send_cdc(char*, int);

void init(void) {
  CNPUBSET = 1 << 8;		// enable B8 pull-up resistor
  TRISBCLR = 1 << 9;            // clear tris -> set B9 as output
  _CP0_SET_COMPARE(MS);         // set core timer interrupt condition
  SYSKEY = 0;                   // ensure OSCCON is locked
  SYSKEY = 0xAA996655;          // unlock sequence
  SYSKEY = 0x556699AA;
  OSCCONSET = _OSCCON_SLPEN_MASK;	// enter Sleep mode at WAIT ins
  CFGCONbits.IOLOCK = 0;        // allow write
  SDI1R = 0b0000;		// RA1 - SDI1
  RPB13R = 0b0011;		// RB13 - SDO1
  RPB1R = 0b0100;		// RB1 - SDO2
  RPB5R = 0b0100;		// RB5 - SDO2
  INT4R = 0b0001;		// RB3 - INT4
  RPC5R = 0b0101;		// RC5 - OC1
  RPC9R = 0b0101;		// RC9 - OC3
  IC3R = 0b0111;		// RA9 - IC3
  CFGCONbits.IOLOCK = 1;        // forbidden write
  SYSKEY = 0;                   // relock
  ANSELACLR = 1 << 1;		// RA1
  ANSELBCLR = 0b1100;		// RB2 & RB3
  ANSELCCLR = 1 << 3;		// RC3
  TRISACLR = 0x481;		// RA0(RESET) RA7(D/CX) RA0(BL)
  TRISBCLR = TRISBSET = 1;
  TRISCCLR = TRISCSET = 0x90;	// RC7, RC4
  IPC0bits.CTIP = 1;            // core timer interrupt at lowest priority
  IEC0bits.CTIE = 1;            // enable core timer interrupt
  OC3CONbits.OCM = 0b110;	// PWM mode, Fault pin disabled
  OC3R = OC3RS = 20000;		// PR2 = 39999
  INTCONSET = _INTCON_MVEC_MASK;
  __builtin_enable_interrupts();        // global interrupt enable
}

void toggle_LED(void) { LATBINV = 1 << 9; }

static volatile unsigned tick;

void poll(unsigned timestamp);  // it will be called when timestamp changes

static void loop(unsigned t) {
  static char *buf, buffer[64];
  static int len;
  if (buf) {
    if (send_cdc(buffer, len)) buf = 0;
  } else {
    if (send_cdc(0, 0))
      if ((len = read_cdc(buf)))
        for (int i = 0; i < len; i++) buffer[i] = buf[i];
  }
  static unsigned tick;
  if (tick != t) poll(tick = t);
}

bool wait(unsigned t) {
  unsigned u = tick;
  for (loop(u); t--; u = tick) while (u == tick) loop(u);
  return true;
}

bool ticking(void) {
  static unsigned t;
  if (t == tick) return false;
  t = tick; return true;
}

extern "C"
__attribute__((interrupt(ipl1soft), vector(_CORE_TIMER_VECTOR), nomips16))
void ctisr(void) {
  _CP0_SET_COMPARE(_CP0_GET_COMPARE() + MS);    // next interrupt at 1 ms
  tick++;                                       // keep track of time
  IFS0bits.CTIF = 0;                            // clear flag
}

void on_switch_change(bool);
bool read_switch(void) {
  static int state = 0x100;	// assume switch is not pressed
  if (state != (PORTB & 0x100)) on_switch_change(state ^= 0x100);
  return state;
}

void buzzer(int s) {
  PR2 = 39999;	// 1 KHz
  OC3CONbits.ON = s;
  T2CONbits.ON = s;
}
