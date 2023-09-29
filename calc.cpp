#include "menu.h"
#include "gram.h"
#include "tft.h"

extern Image calcDisplay, *largeButton;
extern Font *fnt24, *fnt28, *fnt68;

namespace
{

Image *displayImg = &calcDisplay;

bool error;
int button = -1;

class Display {
public:
  Display(void) { clear_buffer(); }
  void draw(void) {
    char data[12800];
    gram::set(320, 80, data);
    gram::draw(displayImg, 0, 0);
    if (error) gram::print(fnt24, RED, 5, "ERROR", 125, 28);
    else gram::print(fnt28, BLACK, 11, buffer, 50, 26);
    if (button > 0) gram::fill(GREEN, 280, 30, 20, 20);
    tft_write_data(400, data, 12800);
  }
  void clear_buffer(void) {
    for (int i = 0; i < SIZE - 1; i++) buffer[i] = 32;
    buffer[SIZE - 1] = '0';
    dotted = false; editing = true;
  }
  void key(int k) {
    if ((error)||(k > 11)||(k < 0)) return;
    if ((!k)&&(dotted)&&(editing)) return;
    if (k == 1) { 	// +/-
      int i = 0;
      while (buffer[i++] == 32);
      if (buffer[--i] == '-') buffer[i] = 32;
      else {
        if ((i == SIZE - 1)&&(buffer[i] == '0')) return;
        if (i) buffer[--i] = '-';
        else { if (dotted) del('-'); }
      }
      value = -value;
    } else {
      if (!editing) clear_buffer();
      if (buffer[0] == 32) insert(k + '.');
    }
  }
  double getValue(void) {
    if (editing) {
      value = 0;
      double multi = 1;
      for (int i = SIZE - 1; i >= 0; i--) {
        if (buffer[i] == 32) break;
        if (buffer[i] == '.') {
          value /= multi;
          multi = 1;
          continue;
        }
        if (buffer[i] == '-') { value = -value; break; }
        value += (buffer[i] - '0') * multi;
        multi *= 10;
      }
      editing = false;
    }
    return value;
  }
  void setValue(double f) {
    value = f;
    clear_buffer();
    editing = false;
    double abs = f < 0 ? -f : f;
    int integer = abs;
    abs -= integer;
    char *p = &buffer[SIZE - 1];
    if (integer)
      while (integer) {
        int i = integer / 10;
        *p-- = integer - i * 10 + '0';
        integer = i;
      }
    else p--;
    if (f < 0) *p = '-';
    insert('.'); dotted = true;
    while (buffer[0] == 32) {
      int i = (abs *= 10);
      insert(i + '0');
      abs -= i;
    }
    while (buffer[SIZE - 1] == '0') del(32);
    if (buffer[SIZE - 1] == '.') { del(32); dotted = false; }
  }
private:
  enum { SIZE=11 };
  char buffer[SIZE];
  bool dotted, editing;
  double value;
  void del(char c) {
    if (buffer[SIZE - 1] == '.') dotted = false;
    for (int i = SIZE - 1; i > 0; i--) buffer[i] = buffer[i - 1];
    buffer[0] = c;
  }
  void insert(char c) {
    if (c == '.') dotted = true;
    else if ((buffer[SIZE - 2] == 32)&&(buffer[SIZE - 1] == '0'))
           buffer[SIZE - 1] = 32;
    for (int i = 0; i < SIZE - 1; i++) buffer[i] = buffer[i + 1];
    buffer[SIZE - 1] = c;
  }
} display;

const struct { int rank; bool(*op)(double&, double); } table[] = {
  { 0, 0 },
  { 0, [](double &v1, double v2) -> bool { v1 = v1 + v2; return true; } },
  { 0, [](double &v1, double v2) -> bool { v1 = v1 - v2; return true; } },
  { 1, [](double &v1, double v2) -> bool { v1 = v1 * v2; return true; } },
  { 1, [](double &v1, double v2) ->
       bool {
          if (v2 == 0) return false;
          v1 = v1 / v2; return true;
       }
  }
};

struct {
  bool key(int k) {
    if (k < 14) return true;
    if ((k -= 14) > 4) return true;
    if (op2) {
      if (!(*table[op1].op)(v1, display.getValue())) return false;
      display.setValue(v1);
      op1 = op2; op2 = 0; v1 = v2;
    }
    if (op1) {
      if (table[k].rank > table[op1].rank) { op2 = op1; v2 = v1; }
      else {
        if (!(*table[op1].op)(v1, display.getValue())) return false;
        display.setValue(v1);
      }
    }
    op1 = k;
    v1 = display.getValue();
    return true;
  }
  void clear(void) { op1 = op2 = 0; }
private:
  int op1, op2; double v1, v2;
} calculate;
 
void draw_buttons(char *p) {
  char *data = gram::paste();
  gram::print(fnt28, WHITE, p[0], 28, 26);
  gram::print(fnt28, WHITE, p[1], 108, 26);
  gram::print(fnt28, WHITE, p[2], 188, 26);
  gram::print(fnt68, WHITE, p[3], 248, 6);
  tft_write_data(data, 12800);
}

void draw_panel(void) {
  char data[25600];
  gram::set(320, 80, data);
  gram::fill(TEAL);
  gram::draw(largeButton, 2, 5);
  gram::draw(largeButton, 82, 5);
  gram::draw(largeButton, 162, 5);
  gram::draw(largeButton, 242, 5);
  gram::copy();
  gram::print(fnt28, WHITE, '0', 28, 26);
  gram::print(fnt28, WHITE, '.', 108, 26);
  gram::print(fnt68, WHITE, '=', 168, 6);
  gram::print(fnt68, WHITE, '+', 248, 6);
  tft_paset(0, 479);
  tft_cmd(WRITE_PIXEL_FORMAT, 1);
  tft_cmd(MEMORY_WRITE);
  tft_write_data(data, 12800);
  draw_buttons("123-");
  draw_buttons("456*");
  draw_buttons("789/");
  gram::paste();
  gram::print(fnt68, WHITE, '<', 8, 6);
  gram::print(fnt68, WHITE, '.', 88, 6);
  gram::print(fnt68, MAROON, ',', 168, 6);
  gram::print(fnt68, MAROON, ',', 248, 6);
  gram::print(fnt24, WHITE, 'C', 190, 28);
  gram::print(fnt24, WHITE, 2, "AC", 264, 28);
  tft_write_data(data, 12800);
  tft_cmd(WRITE_PIXEL_FORMAT, 6);
}

void clear_all(void) { display.clear_buffer(); calculate.clear(); }

}//anonymous

char *calc(Cmd cmd, int x, int y) {
  if (cmd == DRAW) {
    draw_panel();
    display.draw();
  }
  if (cmd == DOWN) {
    if (error) { error = false; clear_all(); }
    if (y < 400) {
      const char table[20] = { 2, 0, 14, 15,
                               3, 4, 5, 16,
                               6, 7, 8, 17, 
                               9, 10, 11, 18,
                               1, 13, 12, 19 };
      y = table[((y / 80) << 2) + (x / 80)];
    } else y = -2;
    if (button != y) {
      if (y == 12) display.clear_buffer();
      if (y == 13) {	// 1/x
        double f = display.getValue();
        if (f == 0) error = true;
        else display.setValue(1 / f);
      }
      if (y == 19) clear_all();
      display.key(button = y);
      if (!calculate.key(y)) error = true;
      display.draw();
    }
  }
  if (cmd == UP) { button = -1; display.draw(); if (y > 400) menu(0); }
  return "CALCULATOR";
}
