#include "menu.h"
#include "gram.h"
#include "tft.h"

extern Image remoteDisplay, remoteButton;
extern Font font16, *fnt24;
Font *fnt16 = &font16;
Image *remoteDisplayImg = &remoteDisplay;

void rc6(unsigned char*, unsigned a, unsigned c);
void ir_send(unsigned char*);

namespace
{

int button = -1;
unsigned char ir_buf[96];

Image *buttonImg = &remoteButton;

const unsigned char code[] = {
84, 0, 255, 17,
7, 8, 9, 16,
4, 5, 6, 255,
1, 2, 3, 255,
255, 255, 255, 255,
255, 89, 255, 255,
90, 92, 91, 255,
12, 88, 255, 63
};

const struct Row {
  struct __attribute__((__packed__)) { short color; char size, label[13]; }
    labels[4];
  void print(void) {
    for (int i = 0; i < 4; i++) {
      if (labels[i].size) {
        Font *fnt = labels[i].size > 4 ? fnt16 : fnt24;
        gram::print(fnt,
                    (Color)labels[i].color,
                    labels[i].size,
                    labels[i].label,
                    i * 80 + ((80 - labels[i].size * fnt->width) >> 1),
                    (50 - fnt->height) >> 1);
      }
    }
  }
} row[8] = {
  { { { WHITE, 4, "MENU" }, { WHITE, 1, "0" }, {}, { WHITE, 1, "-" } } },
  { { { WHITE, 1, "7" }, { WHITE, 1, "8" }, { WHITE, 1, "9" }, { WHITE, 1, "+" } } },
  { { { WHITE, 1, "4" }, { WHITE, 1, "5" }, { WHITE, 1, "6" }, { WHITE, 1, "v" } } },
  { { { WHITE, 1, "1" }, { WHITE, 1, "2" }, { WHITE, 1, "3" }, { WHITE, 1, "^" } } },
  { { { RED, 4, { 127, 127, 127, 127 } }, { GREEN, 4, { 127, 127, 127, 127 } }, { YELLOW, 4, { 127, 127, 127, 127 } }, { BLUE, 4, { 127, 127, 127, 127 } } } },
  { { {}, { WHITE, 1, "v" }, {}, {} } },
  { { { WHITE, 1, "<" }, { WHITE, 2, "OK" }, { WHITE, 1, ">" }, {} } },
  { { { RED, 3, "PWR" }, { WHITE, 1, "^" }, {}, { WHITE, 3, "SRC" } } }
};

void draw_display(Color c, int size, char *p) {
  char data[10240];
  gram::set(320, 64, data);
  gram::fill(TEAL);
  gram::draw(remoteDisplayImg, 0, 0);
  if (c == WHITE) c = BLUE;
  gram::print(fnt24, c, size, p,
              (320 - size * fnt24->width) >> 1, (48 - fnt24->height) >> 1);
  tft_write_data(416, data, 10240);
}

void draw_panel(void) {
  char data[21120];
  gram::set(320, 66, data);
  gram::fill(TEAL);
  gram::draw(buttonImg, 2, 9);
  gram::draw(buttonImg, 82, 9);
  gram::draw(buttonImg, 162, 9);
  gram::draw(buttonImg, 242, 9);
  gram::copy();
  tft_paset(0, 479);
  tft_cmd(WRITE_PIXEL_FORMAT, 1);
  tft_cmd(MEMORY_WRITE);
  for (int i = 0; i < 8; i++) {
    ((Row*)&row[i])->print();
    tft_write_data(data, 8000);
    gram::paste();
  }
  gram::fill(TEAL);
  tft_write_data(data, 2560);
  tft_cmd(WRITE_PIXEL_FORMAT, 6);
}

}//anonymous

char *remote(Cmd cmd, int x, int y) {
  if (cmd == DRAW) {
    draw_panel();
    draw_display(BLUE, 13, remote(NAME, 0, 0));
  }
  if (cmd == DOWN) {
    int id = -2;
    if (y < 400) { y = y / 50; x = x / 80; id = (y << 2) + x; }
    if (button != id) {
      button = id;
      if (id >= 0) {
        draw_display((Color)row[y].labels[x].color,
                     row[y].labels[x].size,
                     (char*)row[y].labels[x].label);
        if (row[y].labels[x].size) {
          rc6(ir_buf, 4, code[id]);
          ir_send(ir_buf);
        }
      }
    }
  }
  if (cmd == UP) {
    button = -1;
    if (y > 400) menu(0);
    else draw_display(BLUE, 13, remote(NAME, 0, 0));
  }
  return "PHILIPS HI-FI";
}

bool ir_repeat(void) { return button != -1; }
