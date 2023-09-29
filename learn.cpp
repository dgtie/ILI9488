#include <xc.h>
#include "menu.h"
#include "gram.h"
#include "tft.h"

#define SIZE 100	// must be even
#define IR PORTAbits.RA9

void int2str(unsigned, char*);
char *learn(Cmd,int,int);

extern Image *remoteDisplayImg;
extern Font *fnt16, *fnt24;

namespace
{

unsigned short irdata[SIZE];
char phase_data[51];
int ir_index, last, unit;

void normalize(void) {
  unit = 65536;
  for (int i = 1; i < ir_index; i++)
    if (irdata[i] < unit) unit = irdata[i];
  unit = (irdata[2] + irdata[3]) / unit;
  unit = (irdata[2] + irdata[3]) / unit;
  for (int i = 0; i < ir_index; i++)
    irdata[i] = (irdata[i] + (unit >> 1)) / unit;
  irdata[ir_index] = 0;
  if (unit < 20) unit = 0;	// illegal
}

void capture_start(void) {
  ir_index = 0;
  last = TMR3;
  T3CONbits.ON = 1;
  IC3BUF;
  IFS0CLR = _IFS0_IC3IF_MASK;
  IEC0bits.IC3IE = 1;
}

void capture_stop(void) {
  T3CONbits.ON = 0;
  IEC0bits.IC3IE = 0;
}

struct {
  void draw(unsigned short *p) {
    y = 0;
    char data[(WIDTH >> 1) * HEIGHT];
    gram::set(WIDTH, HEIGHT, data);
    gram::fill(TEAL);
    while (1) {
      if (!mark(p[0])) break;
      if (!space(p[1])) break;
      p += 2;
    }
    tft_caset(10, WIDTH + 9);
    tft_write_data(10, data, (WIDTH >> 1) * HEIGHT);
  }
private:
  int y;
  enum { WIDTH=50, HEIGHT=406, COLOR=VIOLET };
  bool mark(int m) {
    if (y + (m << 1) >= HEIGHT) return false;
    gram::fill((Color)COLOR, 0, y, WIDTH, m << 1);
    return (y += m << 1);
  }
  bool space(int s) {
    if (!s) return false;
    while (s > 9) if (!ten(s -= 10)) return false;
    if (s) {
      if (y + (s << 1) >= HEIGHT) return false;
      gram::fill((Color)COLOR, WIDTH - 2, y, 2, s << 1);
      y += s << 1;
    }
    return true;
  }
  bool ten(int) {
    if (y + 4 >= HEIGHT) return false;
    gram::fill((Color)COLOR, WIDTH - 6, y + 2, 2, 2);
    return (y += 6);
  }
} signal;

struct {
  void data(void) {
    int i = 0, idle;
    index = count = 0;
    while (i < 51) {
      if ((phase_data[i] = bit2()) > 3) break;
      if ((phase_data[i] == 1)&&(idle > 7)) {
        phase_data[i] = 4; if (i == 50) break;
        if ((phase_data[++i] = bit1()) > 3) break;
      }
      if (phase_data[i++]) idle = 0; else idle++;
    }
    if (i < 49) phase_data[++i] = 6;
  }
  int read(int pos, int size) {
    int v = 0;
    for (int i = pos; i < pos + size; i++) {
      v <<= 1; 
      if (phase_data[i] == 1) continue;
      if (phase_data[i] == 2) v++;
      else return -1;
    }
    return v;
  }
  void draw(void) {
    char data[(WIDTH >> 1) * HEIGHT];
    gram::set(WIDTH, HEIGHT, data);
    gram::fill(TEAL);
    for (int i = 0; i < 51; i++) {
      if (phase_data[i] == 6) break;
      gram::draw((Image*)table[phase_data[i]], 0, i * 8);
    }
    tft_caset(80, WIDTH + 79);
    tft_write_data(10, data, (WIDTH >> 1) * HEIGHT);
  }
private:
  enum { WIDTH=20, HEIGHT=406 };
  int index, count;
  const struct { short width, height; unsigned char rle[4]; } sq0
  = { 6, 6, { 255, 0, 17, 62 } };
  const struct { short width, height; unsigned char rle[4]; } sq1
  = { 6, 6, { 192, 0, 17, 62 } };
  const struct { short width, height; unsigned char rle[28]; } sq00
  = { 14, 6, { 255, 255, 255, 63, 255, 0, 5, 63, 255, 0, 5, 63, 255, 0, 5, 63, 
               255, 0, 5, 63, 255, 0, 5, 63, 255, 0, 2, 62 } };
  const struct { short width, height; unsigned char rle[43]; } sq01
  = { 20, 6, { 255, 255, 255, 63, 192, 0, 5, 255, 255, 255, 63, 192, 0, 5,
               255, 255, 255, 63, 192, 0, 5, 255, 255, 255, 63, 192, 0, 5,
               255, 255, 255, 63, 192, 0, 5, 255, 255, 255, 63, 192, 0, 5,
               62 } };
  const struct { short width, height; unsigned char rle[43]; } sq10
  = { 20, 6, { 192, 192, 192, 63, 255, 0, 5, 192, 192, 192, 63, 255, 0, 5,
               192, 192, 192, 63, 255, 0, 5, 192, 192, 192, 63, 255, 0, 5,
               192, 192, 192, 63, 255, 0, 5, 192, 192, 192, 63, 255, 0, 5,
               62 } };
  const struct { short width, height; unsigned char rle[28]; } sq11
  = { 14, 6, { 192, 192, 192, 63, 192, 0, 5, 63, 192, 0, 5, 63, 192, 0, 5, 63, 
               192, 0, 5, 63, 192, 0, 5, 63, 192, 0, 2, 62 } };
  const Image* table[6]
  = { (Image*)&sq00, (Image*)&sq01, (Image*)&sq10, (Image*)&sq11,
      (Image*)&sq0, (Image*)&sq1 };
  int bit1(void) {
    if (!count) if (!(count = irdata[index++])) return 5;
    count--;
    return index & 1 ? 3 : 2;
  }
  int bit2(void) {
    int bit = bit1();
    if (bit == 5) return 6;
    int b = bit1();
    if (b != 5) bit = (bit << 1) ^ b;
    return bit ^ 6;
  }
} phaseM;

struct {
  void draw(void) {
    char data[(WIDTH >> 1) * HEIGHT];
    gram::set(WIDTH, HEIGHT, data);
    gram::fill(TEAL);
    for (int i = 0; i < 68; i++) {
      if (!irdata[i]) break;
      int w = irdata[i] * 6;
      if (!((i & 1)&&(w > WIDTH))) {
        if (w > WIDTH) w = WIDTH;
        gram::fill(i & 1 ? WHITE : BLACK, 0, i * 6, w, 4);
      }
    }
    for (int i = 4; i < WIDTH; i += 6) gram::fill(TEAL, i, 0, 2, HEIGHT);
    tft_caset(120, WIDTH + 119);
    tft_write_data(10, data, (WIDTH >> 1) * HEIGHT);
  }
private:
  enum { WIDTH=124, HEIGHT=406 };
} widthM;

int sirc20(void), nec(void), sharp(void), rc6(void);

struct {
  int parameters[4];
  void decode(void) {
    int i, n, s; char text[] = "536";
                       //       012
    for (i = 0; i < TABLESIZE; i++) if ((n = (*table[i].func)())) break;
    s = table[i].size;
    char data[6720];
    gram::set(150, 48, data);
    gram::fill(WHITE);
    if (n) gram::print(fnt24, BLACK, s, (char*)table[i].name,
                       (150 - s * 14) >> 1, 12);
    else gram::print(fnt24, RED, "UNKNOWN", 26, 12);
    tft_caset(160, 309);
    tft_write_data(70, data, 3600);
    gram::set(70, 192);
    gram::fill(TEAL);
    if (n) {
      gram::fill(WHITE, 0, 0, 70, n * 48);
      for (int i = 0; i < n; i++) {
        text[0] = text[1] = 32;
        int2str(parameters[i], &text[2]);
        gram::print(fnt24, BLACK, 3, text, 14, 12 + i * 48);
      }
    }
    tft_caset(240, 309);
    tft_write_data(120, data, 6720);
  }
private:
  enum { TABLESIZE=4 };
  const struct { int size; char name[8]; int(*func)(void); }
    table[TABLESIZE] = {
    { 7, "SIRC 20", &sirc20 }, { 3, "NEC", &nec }, { 5, "SHARP", &sharp },
    { 3, "RC6", &rc6 }
  };
} decoder;

int sirc20(void) {
// return number of parameters
  if (irdata[0] != 4) return 0;
  if ((irdata[41])&&(irdata[41] < 5)) return 0;
  for (int i = 2; i < 41; i += 2)
    if ((irdata[i] != 1)&&(irdata[i] != 2)) return 0;
  int b = 0;
  for (int i = 40; i > 1; i -= 2) { b <<= 1; if (irdata[i] == 2) b++; }
  decoder.parameters[0] = b & 0x7f; b >>= 7;
  decoder.parameters[1] = b & 0x1f; b >>= 5;
  decoder.parameters[2] = b;
  return 3;
}

int nec(void) {
  if ((irdata[0] != 16)||(irdata[1] != 8)) return 0;
  if ((irdata[67])&&(irdata[67] < 5)) return 0;
  for (int i = 3; i < 67; i += 2)
    if ((irdata[i] != 1)&&(irdata[i] != 3)) return 0;
  unsigned b = 0;
  for (int i = 65; i > 1; i -= 2) { b <<= 1; if (irdata[i] == 3) b++; }
  decoder.parameters[0] = b & 0xff; b >>= 8;
  decoder.parameters[1] = b & 0xff; b >>= 8;
  decoder.parameters[2] = b & 0xff; b >>= 8;
  decoder.parameters[3] = b;
  return 4;
}

int sharp(void) {
  if ((irdata[0] != 1)||(irdata[2] != 1)) return 0;
  if ((irdata[31])&&(irdata[31] < 5)) return 0;
  for (int i = 1; i < 31; i += 2)
    if ((irdata[i] != 2)&&(irdata[i] != 5)) return 0;
  int b = 0;
  for (int i = 25; i > 0; i -= 2) { b <<= 1; if (irdata[i] == 5) b++; }
  decoder.parameters[0] = b & 0x1f; b >>= 5;
  decoder.parameters[1] = b;
  return 2;
}

int rc6(void) {
  if ((irdata[0] != 6)||(irdata[1] != 2)) return 0;
  int m = phaseM.read(4, 4);
  if ((m < 0)||(!(m & 8))) return 0;
  int c = phaseM.read(10, 8);
  int i = phaseM.read(18, 8);
  if ((i < 0)||(c < 0)) return 0;
  decoder.parameters[0] = m & 7;
  decoder.parameters[1] = c;
  decoder.parameters[2] = i;
  return 3;
}

void draw_title(void) {
  char data[8640];
  gram::set(320, 54, data);
  gram::fill(TEAL);
  gram::draw(remoteDisplayImg, 0, 0);
  gram::print(fnt24, BLUE, learn(NAME, 0, 0), 104, 12);
  tft_write_data(426, data, 8640);
}

void draw(void) {
  char text[] = "unit =   0 us";
  //             01234567890123
  draw_title();
  if (unit) {
    signal.draw(irdata);
    phaseM.draw();
    widthM.draw();
    int2str((unit * 64) / 10, &text[9]);
    char data[2400];
    gram::set(150, 32, data);
    gram::fill(WHITE);
    gram::print(fnt16, BLACK, text, 10, 8);
    tft_caset(160, 309);
    tft_write_data(30, data, 2400);
    decoder.decode();
    tft_caset(0, 319);
  }
}

void draw_background(void) {
  char data[22720];
  gram::set(320, 142, data);
  gram::fill(TEAL);
  tft_paset(0, 479);
  tft_cmd(WRITE_PIXEL_FORMAT, 1);
  tft_cmd(MEMORY_WRITE);
  tft_write_data(data, 22720);
  tft_write_data(data, 22720);
  tft_write_data(data, 22720);
  tft_cmd(WRITE_PIXEL_FORMAT, 6);
}

} //anonymous

char *learn(Cmd cmd, int x, int y) {
  if (cmd == DRAW) {
    draw_background();
    draw();
    capture_start();
  }
  if (cmd == MAIN) {
    static int index, count;
    if (++count > 150) {
      count = 0;
      if (ir_index) {
        if (index == ir_index) {
          capture_stop();
          normalize();
          phaseM.data();
          draw();
          capture_start();
          index = 0;
        } else index = ir_index;
        return 0;
      }
    }
  }
  if (cmd == UP) {
    if (y > 429) { capture_stop(); menu(0); }
  }
  return "LEARN IR";
}

extern "C"
void __attribute__((interrupt(ipl1soft), vector(_INPUT_CAPTURE_3_VECTOR), nomips16))
ic3(void) {
  LATBINV = 1 << 9;
  irdata[ir_index] = IC3BUF - last;
  last += irdata[ir_index++];
  if ((ir_index == SIZE)||((ir_index == 1)&&(!IR))) ir_index--;
  IFS0CLR = _IFS0_IC3IF_MASK;
}
