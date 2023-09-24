#include <xc.h>

/*
 *  SCK2 - RB15 (IFS1, IEC1, IPC9)
 *  CSX  - RA0
 *  SDO2 - RB1
 *  SDI2 - RB2
 *  INT4 - RB3 (IFS0, IEC0, IPC4)
 */

#define CS_UP LATBSET = 1
#define CS_DN LATBCLR = 1
#define TOUCH_OFF PORTBbits.RB3

#define X_POS 0xD0	// 16 bit, differential, IRQ enabled
#define Y_POS 0x90
#define Z1    0xB0
#define Z2    0xC0

namespace
{

int idx;
const short cmds[] = { 0, X_POS, Y_POS, Z1, Z2 };
short results[5];		// results[4] holds 1st dummy word read
				// results[0] holds X position

} // anonymous

void enable_int4(void) {
  IFS0CLR = _IFS0_INT4IF_MASK;
  IEC0bits.INT4IE = 1;
}

void xpt_init(void) {
  SYSKEY = 0;           	// ensure OSCCON is locked
  SYSKEY = 0xAA996655;  	// unlock sequence
  SYSKEY = 0x556699AA;
  CFGCONbits.IOLOCK = 0;	// allow write
  SDI2R = 0b0100;		// RB2 - SDI2
  CFGCONbits.IOLOCK = 1;        // forbidden write
  SYSKEY = 0;                   // relock
  CS_UP;
  IPC4bits.INT4IP = 1;
  IPC9bits.SPI2IP = 1;			// interrupt priority
  INTCONbits.INT4EP = 1;		// rising edge
  SPI2CON = SPICON_MSTEN | SPICON_CKE | SPICON_MODE16;
					// master, CKP=SMP=0, CKP=1, 16-bit
  SPI2BRG = 9;				// 2 MHz
  IFS1CLR = _IFS1_SPI2RXIF_MASK;	// clear receive IF
  IEC1bits.SPI2RXIE = 1;		// enable receive IE
  SPI2CONbits.ON = 1;			// turn on SPI2
}

void xpt_deinit(void) {
  IEC1bits.SPI2RXIE = 0;
  SPI2CONbits.ON = 0;
  SPI2CONbits.MODE16 = 0;
}

void process_xpt(int x, int y), touch_off(void);
bool xpt(void) {		// call me every milli-second
  static bool was_touching = false;	// initial pen is up
  static int count;
  if (!SPI2CONbits.MODE16) return false;	// xpt is disabled
  if (++count < 64) return false;
  count = 0;
  if (was_touching) {
    results[0] >>= 3;			// X, LSB is at bit-3
    results[1] >>= 3;			// Y
    results[2] >>= 3;			// Z1
    results[3] >>= 3;			// Z2
    process_xpt(results[0], results[1]);
  }
  if (TOUCH_OFF) {			// if not touching
    if (was_touching) { was_touching = false; touch_off(); }
    return false;
  } else {
    was_touching = true;		// mark pen is down
    CS_DN;				// select xpt2046
    SPI2BUF = cmds[idx = sizeof(cmds) - 1];	// send commands string
    return true;
  }
}

extern "C"
void __attribute__((interrupt(ipl1soft), vector(_SPI_2_VECTOR), nomips16))
spi2(void) {
  results[idx] = SPI2BUF;	// save result byte
  if (idx) SPI2BUF = cmds[--idx];	// send command word, if any
  else CS_UP;			// CSX up when done
  IFS1CLR = _IFS1_SPI2RXIF_MASK;	// clear IF
}

extern "C"
void __attribute__((interrupt(ipl1soft), vector(_EXTERNAL_4_VECTOR), nomips16))
int4(void) {
  IFS0CLR = _IFS0_INT4IF_MASK;
  IEC0bits.INT4IE = 0;
}
