#include "menu.h"
#include "gram.h"
#include "tft.h"

void buzzer(int);

extern Image timerButton, timerDisplay;
Image *largeButton = &timerButton;
extern Font font28, font68;
Font *fnt28 = &font28;
Font *fnt68 = &font68;

namespace
{

enum { STOPPED, COUNTING, ALARM } state;

struct { Image *image; char text[4];
  void draw(Color c) {
    char data[24000];
    gram::set(320, 150, data);
    gram::fill(c);
    gram::draw(image, 0, 0);
    gram::print(fnt68, BLACK, 2, text, 20, 41);
    gram::print(fnt68, BLACK, 2, &text[2], 180, 41);
    tft_write_data(330, data, 24000);
  }
  void key(char k) {
    if ((k < '0')||(k > '9')) return;
    for (int i = 0; i < 3; i++) text[i] = text[i + 1];
    text[3] = k;
  }
  void time(int s) {
    char *p = &text[3];
    int t;
    t = s / 10; *p-- = s - t * 10 + '0';
    s = t / 6; *p-- = t - s * 6 + '0';
    t = s / 10; *p-- = s - t * 10 + '0';
    s = t / 10; *p-- = t - s * 10 + '0';
  }
  int time(void) {
    int t = (text[0] - '0') * 10 + (text[1] - '0');
    t *= 60;
    t += (text[2] - '0') * 10 + (text[3] - '0');
    return t;
  }
} display = { &timerDisplay, { 48, 48, 48, 48 } };

void draw_buttons(char *p) {
  char *data = gram::paste();
  gram::print(fnt28, WHITE, p[0], 66, 26);
  gram::print(fnt28, WHITE, p[1], 150, 26);
  gram::print(fnt28, WHITE, p[2], 234, 26);
  tft_write_data(data, 12800);
}

void draw_panel(void) {
  char data[25600];
  gram::set(320, 80, data);
  gram::fill(TEAL);
  gram::draw(largeButton, 40, 5);
  gram::draw(largeButton, 124, 5);
  gram::draw(largeButton, 208, 5);
  gram::copy();
  gram::print(fnt68, MAROON, 44, 46, 5);
  gram::print(fnt68, MAROON, 44, 214, 5);
  gram::print(fnt68, WHITE, ':', 46, 5);
  gram::print(fnt28, WHITE, '0', 150, 26);
  gram::print(fnt68, WHITE, ';', 214, 5);
  tft_paset(0, 479);
  tft_cmd(WRITE_PIXEL_FORMAT, 1);
  tft_cmd(MEMORY_WRITE);
  tft_write_data(data, 12800);
  draw_buttons("123");
  draw_buttons("456");
  draw_buttons("789");
  gram::fill(TEAL);
  tft_write_data(data, 1600);
  tft_cmd(WRITE_PIXEL_FORMAT, 6);
}

void alarm_on(void) { buzzer(1); display.draw(RED); state = ALARM; }

void alarm_off(void) { buzzer(0); display.draw(LIGHTGREEN); state = STOPPED; }

void clear(void) { display.time(0); alarm_off(); }

} //anonymous

char *timer(Cmd cmd, int x, int y) {
  static int button = -1, time;
  if (cmd == DRAW) {
    draw_panel();
    display.draw(LIGHTGREEN);
  }
  if (cmd == MAIN) {
    if (state == ALARM) return 0;
    if (state == COUNTING) {
      if (!--time) alarm_on();
      else {
        int r = time - (time / 1000) * 1000;
        if (r == 999) {
          display.time(time / 1000);
          display.draw(LIGHTGREEN);
        }
      }
      return 0;
    }
  }
  if (cmd == DOWN) {
    if (state == ALARM) alarm_off();
    else {
      int btn;
      if ((x > 34)&&(x < 286)&&(y < 320)) btn = (y / 80) * 3 + (x - 34) / 84;
      else btn = -2;
      if (button != btn) {
        if ((button = btn) == 2) clear();
        else {
          if ((state == STOPPED)&&(btn >= 0)) {
            if (btn) display.key(btn == 1 ? '0' : btn - 2 + '0');
            else {
              if ((time = display.time())) state = COUNTING;
              display.time(time);
              time *= 1000;
            }
          }
        }
        display.draw(LIGHTGREEN);
      }
    }
  }
  if (cmd == UP) {
    if ((y > 330)&&(state == STOPPED)) menu(0);
    button = -1;
  }
  return "TIMER";
}
