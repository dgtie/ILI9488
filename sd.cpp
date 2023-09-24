/* References:
 * elm-chan.org/docs/mmc/mmc_e.html
 *
 * ONLY supports:
 * SD Ver.2 & SD Ver.1
 */

#include <xc.h>
#include "sd.h"

/*  SD CARD
 *  *******
 *  SCK2 - RB15 (IFS1, IEC1, IPC9)
 *  CSX  - RC4
 *  MOSI - RB5
 *  MISO - RC3
 *
 *  DMA2 - write spi (IFS1, IEC1, IPC10)
 *  DMA3 - read spi (IFS1, IEC1, IPC10)
 */

#define Virt2Phys(addr) ((int)addr & 0x1fffffff)

#define CS_UP LATCSET = 1 << 4
#define CS_DN LATCCLR = 1 << 4

namespace
{

STATE state = DISCON;

struct __attribute__((__packed__)) {
  char checksum; int param; char command;
} command_packet;

DataPacket *data_packet;

int error, address;

int read_resp(void) {
  SPI2STATCLR = SPISTAT_SPIROV;	// clear read overflow
  int r1;
  for (int i = 0; i < 9; i++) {
    while (!SPI2STATbits.SPIRBF);
    r1 = SPI2BUF;
    if (!(r1 & 0x80)) break;
    SPI2BUF = -1;
  }
  return r1 & 255;
}

int cmd(char c, int param) {
  command_packet.command = 0x40 | c;
  command_packet.param = param;
  command_packet.checksum = 0;
  char *array = &command_packet.checksum;
  unsigned char acc = 0;
  int i = 6;
  while (i--) {
    c = array[i];
    for (int j = 0; j < 8; j++) {
      bool b = acc & 0x80;
      acc <<= 1;
      if (c & 0x80) acc++;
      c <<= 1;
      if (b) acc ^= 0x12;	// CRC7
    }
  }
  command_packet.checksum = acc | 1;	// add stop bit
  i = 6;
  while (i--) {
    while (!SPI2STATbits.SPITBE);
    SPI2BUF = array[i];
  }
  return read_resp();
}

int read_param(void) {
  unsigned r;
  for (int i = 0; i < 4; i++) {
    r <<= 8;
    SPI2BUF = -1;
    while(!SPI2STATbits.SPIRBF);
    r |= SPI2BUF & 255;
  }
  return r;
}

unsigned short crc16(char *p) {
  unsigned short acc = 0;
  for (int i = 0; i < 514; i++) {
    unsigned char c = p[i];
    for (int j = 0; j < 8; j++) {
      bool b = acc & 0x8000;
      acc <<= 1;
      if (c & 0x80) acc++;
      c <<= 1;
      if (b) acc ^= 0x1021;
    }
  }
  return acc;
}

bool ready(void) { return (state==READY)||(state==ERROR); }

} //anonymous

STATE sd_state(void) { return state; }

void sd_buffer(DataPacket *d) { data_packet = d; d->ff = -1; }

void sd_init(void) {
  SYSKEY = 0;           	// ensure OSCCON is locked
  SYSKEY = 0xAA996655;  	// unlock sequence
  SYSKEY = 0x556699AA;
  CFGCONbits.IOLOCK = 0;	// allow write
  SDI2R = 0b0111;		// RC3 - SDI2
  CFGCONbits.IOLOCK = 1;        // forbidden write
  SYSKEY = 0;                   // relock
  CNPUCSET = 1 << 3;	// MISO pull up
  IPC10bits.DMA2IP = 1;
  IPC10bits.DMA3IP = 1;
  IEC1bits.DMA3IE = 1;
  DCH3SSA = DCH2DSA = Virt2Phys(&SPI2BUF);
  DCH3SSIZ = DCH3CSIZ = DCH2DSIZ = DCH2CSIZ = 1;
  DCH2ECONbits.CHSIRQ = _SPI2_TX_IRQ;
  DCH2ECONbits.SIRQEN = 1;
  DCH3ECONbits.CHSIRQ = _SPI2_RX_IRQ;
  DCH3ECONbits.SIRQEN = 1;
  SPI2CON = SPICON_MSTEN | SPICON_CKE;
  SPI2BRG = 9;				// 2 MHz
  SPI2CONbits.ON = 1;
}

void sd_deinit(void) {
  DCH2ECONbits.SIRQEN = 0;
  DCH3ECONbits.SIRQEN = 0;
  IEC1bits.DMA3IE = 0;
  SPI2CONbits.ON = 0;
  state = DISCON;
}

int sd_connect(void) {
  state = DISCON;
  CS_UP;
  for (int i = 0; i < 10; i++) {	// dummy clock > 74
    while (!SPI2STATbits.SPITBE);
    SPI2BUF = -1;
  }
  CS_DN; int r1 = cmd(0, 0); CS_UP;
  if (r1 != 1) return error = 0x100 | r1;
  CS_DN;
  r1 = cmd(8, 0x1aa);
  int param = read_param();
  CS_UP;
  if (r1 & 4) param = 0;		// SD Ver.1
  else {				// SD Ver.2
    if (param != 0x1aa) return error = 0x200 | r1;
    param = 0x40000000;
  }
  CS_DN;
  for (int i = 0; i < 512; i++) {
    cmd(55, 0);
    r1 = cmd(41, param);
    if (!r1) break;
  }
  CS_UP;
  if (r1) return error = 0x300 | r1;
  state = READY;
  if (param) {
    CS_DN;
    cmd(58, 0);
    param = read_param();
    CS_UP;
    if (param & 0x40000000) return address = 0;	// block address
  }
  CS_DN; cmd(16, 512); CS_UP;
  address = 9;		// byte address
  return 0;
}

bool sd_read(int addr) {
  if (!ready()) return false;
  state = READ;
  CS_DN;
  int r1 = cmd(17, addr << address);
  int cnt = 0;
  if (!r1) 
    while ((r1 != 254)&&(cnt++ < 256)) {
      SPI2BUF = -1;
      while (!SPI2STATbits.SPIRBF);
      r1 = SPI2BUF;
    }
  if (r1 == 254) {
    DCH2SSA = Virt2Phys(data_packet->idle);
    DCH3DSA = Virt2Phys(data_packet->data);
    DCH2SSIZ = 1;
    DCH3DSIZ = 514;
    DCH2ECONbits.CHAIRQ = _DMA3_IRQ;
    DCH2ECONbits.AIRQEN = 1;
    DCH2CONbits.CHAEN = 1;
    DCH3INTbits.CHBCIF = 0;
    DCH3INTbits.CHBCIE = 1;
    IFS1bits.DMA3IF = 0;
    DCH2CONbits.CHEN = 1;
    DCH3CONbits.CHEN = 1;
    DMACONbits.ON = 1;
  } else {
    CS_UP;
    error = 0x400 | r1;
    state = ERROR;
  }
  return r1 == 254;
}

int sd_write(int addr) {
  if (!ready()) return -1;
  state = WRITE;
  data_packet->token = 0xfe;
  data_packet->checksum = 0;
  int cs = crc16(data_packet->data);
  data_packet->data[512] = cs >> 8;
  data_packet->data[513] = cs;
  CS_DN;
  int r1 = cmd(24, addr << address);
  if (r1) { CS_UP; state = ERROR; return error = 0x500 | r1; }
  DCH2SSA = Virt2Phys(data_packet->idle);
  DCH2SSIZ = 518;
  DCH2CONbits.CHAEN = 0;
  DCH2CONbits.CHEN = 1;
  DCH2INTbits.CHBCIF = 0;
  DCH2INTbits.CHBCIE = 1;
  IFS1bits.DMA2IF = 0;
  IEC1bits.DMA2IE = 1;
  DMACONbits.ON = 1;
  return 0;
}

void sd_poll(unsigned) {
  if ((state == READ)&&(!DMACONbits.ON)) 
    state = crc16(data_packet->data) ? ERROR : READY;
  if ((state == WRITE)&&(!DMACONbits.ON))
    state = read_resp() ? ERROR : BUSY;
  if (state == BUSY) {
    SPI2BUF = -1;
    while (!SPI2STATbits.SPIRBF);
    if (SPI2BUF == 0xff) state = READY;
  }
  if (ready()) CS_UP;
}

extern "C"
__attribute__((interrupt(ipl1soft), vector(_DMA_2_VECTOR), nomips16))
void dma2(void) {
  DMACONbits.ON = 0;
  DCH2INTbits.CHBCIF = 0;
  IFS1bits.DMA2IF = 0;
  IEC1bits.DMA2IE = 0;
}

extern "C"
__attribute__((interrupt(ipl1soft), vector(_DMA_3_VECTOR), nomips16))
void dma3(void) {
  DMACONbits.ON = 0;
  DCH3INTbits.CHBCIF = 0;
  IFS1bits.DMA3IF = 0;
}
