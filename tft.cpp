#include <xc.h>
#include "tft.h"

/*  ILI9488
 *  *******
 *
 *  SCK1  - RB14 (IFS1, IEC1, IPC7)
 *  SDO1  - RB13
 *  SDI1  - RA1
 *  CSX   - RC7
 *  RESET - RA10
 *  D/CX  - RA7
 *
 *  DMA0 - write spi (IFS1, IEC1, IPC10)
 *  DMA1 - read spi (IFS1, IEC1, IPC10)
 *
 *  Pixel format 1: 3 bit color (ie 8 colors)
 *  choose format 1 for low ram requirement (I only have 32k ram)
 *  format 6 uses 3 bytes for 1 pixel
 *  format 1 uses 1 byte for 2 pixels
 */

#define Virt2Phys(addr) ((int)addr & 0x1fffffff)

#define CS LATCbits.LATC7
#define CS_UP LATCSET = 1 << 7
#define CS_DN LATCCLR = 1 << 7
#define RST_UP LATASET = 1 << 10
#define RST_DN LATACLR = 1 << 10
#define DATA LATASET = 1 << 7
#define COMMAND LATACLR = 1 << 7
#define BL_ON LATASET = 1
#define BL_OFF LATACLR = 1

bool wait(unsigned);

namespace
{

int t_cnt, r_cnt;
char *ptr;

void tft(int n, char *p) {
  t_cnt = r_cnt = n; ptr = p;
  COMMAND;
  IEC1bits.SPI1TXIE = 1;
  while (!CS) wait(0);
}

void spi_on(void) {
  SPI1CONbits.ON = 1;
  SPI1BUF;
  IFS1CLR = _IFS1_SPI1RXIF_MASK;	// clear IF
  IEC1bits.SPI1RXIE = 1;
}

} // anonymous

void tft_cmd(char c) { tft(1, &c); }

int tft_cmd(char c, char d) {
  char buf[] = { d, c }; tft(2, buf); return buf[0];
}

void tft_bl_on(void) { BL_ON; }
void tft_bl_off(void) { BL_OFF; }
void tft_on(void) { tft_cmd(SLEEP_OUT); wait(6); tft_cmd(DISPLAY_ON); BL_ON; }
void tft_off(void) {
  BL_OFF; tft_cmd(DISPLAY_OFF); wait(121); tft_cmd(SLEEP_IN);
  LATBCLR = 1 << 9;
}

void tft_read_start(void) {		// tft_read_start();
  CS_DN; COMMAND;			// tft_read_data(..);
  IEC1bits.SPI1RXIE = 0;		// tft_read_data(..);
  while (!SPI1STATbits.SPITBE);		//      ...
  SPI1BUF = MEMORY_READ;		// tft_read_end();
  while (!SPI1STATbits.SPITBE);
  SPI1BUF = 0;				// no other tft_ commands in between
  while (!SPI1STATbits.SPIRBF);
  SPI1BUF;
  DATA;
  while (!SPI1STATbits.SPIRBF);
  SPI1BUF;
}

void tft_read_data(char *p, int n) {	// CS kept low between block read
  DMACONbits.ON = 1;
  DCH0SSA = DCH1DSA = Virt2Phys(p);
  DCH0SSIZ = DCH1DSIZ = n;
  DCH1CONbits.CHEN = 1;
  DCH0CONbits.CHEN = 1;
  while (DCH1CONbits.CHEN) wait(0);
  DMACONbits.ON = 0;
}

void tft_read_end(void) {
  IEC1bits.SPI1RXIE = 1;
  CS_UP;
}


void tft_write_data(char *s, int n) {	// CS may changes between block write
  DMACONbits.ON = 1;
  IEC1bits.SPI1RXIE = 0;
  DCH0SSA = Virt2Phys(s);
  DCH0SSIZ = n;
  CS_DN; DATA;
  DCH0CONbits.CHEN = 1;
  while (DCH0CONbits.CHEN) wait(0);
  DMACONbits.ON = 0;
  // add delay to ensure last byte was sent
  for (int i = 0; i < (SPI1BRG << 4); i++) asm volatile ("nop");
  SPI1CONbits.ON = 0;
  CS_UP;
  spi_on();
}

void tft_write_data(int page, char *s, int n) {
  tft_paset(page, 479);
  tft_cmd(WRITE_PIXEL_FORMAT, 1);
  tft_cmd(MEMORY_WRITE);
  tft_write_data(s, n);
  tft_cmd(WRITE_PIXEL_FORMAT, 6);
}

void tft_caset(short sc, short ec) {
  struct __attribute__((__packed__)) { short s, e; char c; } buf =
  { ec, sc, CASET };
  tft(5, (char*)&buf);
}

void tft_paset(short sp, short ep) {
  struct __attribute__((__packed__)) { short s, e; char c; } buf =
  { ep, sp, PASET };
  tft(5, (char*)&buf);
}

void tft_init(void) {
  CS_UP; RST_UP;
  DCH1SSA = DCH0DSA = Virt2Phys(&SPI1BUF);
  DCH1SSIZ = DCH1CSIZ = DCH0DSIZ = DCH0CSIZ = 1;
  DCH0ECONbits.CHSIRQ = _SPI1_TX_IRQ;
  DCH0ECONbits.SIRQEN = 1;
  DCH1ECONbits.CHSIRQ = _SPI1_RX_IRQ;
  DCH1ECONbits.SIRQEN = 1;
  IPC7bits.SPI1IP = 1;
  SPI1CON = SPICON_MSTEN | SPICON_CKE;
  SPI1BRG = 6;	// 20/(6+1) = 2.857 MHz
  spi_on();
  RST_DN; wait(2); RST_UP; wait(121); 
  tft_on();
  tft_cmd(MEMORY_ACCESS_CONTROL, 0xc0);	// rotate screen 180 deg
}

extern "C"
void __attribute__((interrupt(ipl1soft), vector(_SPI_1_VECTOR), nomips16))
spi1(void) {
  if ((IEC1bits.SPI1TXIE)&&(IFS1bits.SPI1TXIF)) {
    CS_DN;
    SPI1BUF = ptr[--t_cnt];
    IFS1CLR = _IFS1_SPI1TXIF_MASK;	// clear IF
    if (!t_cnt) IEC1bits.SPI1TXIE = 0;
  }
  if ((IEC1bits.SPI1RXIE)&&(IFS1bits.SPI1RXIF)) {
    ptr[--r_cnt] = SPI1BUF;
    IFS1CLR = _IFS1_SPI1RXIF_MASK;	// clear IF
    DATA;
    if (!r_cnt) CS_UP;
  }
}
