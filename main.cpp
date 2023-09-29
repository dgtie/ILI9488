#include "tft.h"
#include "gram.h"
#include "menu.h"
#include "fat32.h"

#define SLEEP 20000	// 20000 = 20 seconds
#define NUM_DEVICE 10

bool send_cdc(char*, int); void int2str(unsigned, char*);
void init(void), toggle_LED(void);
bool wait(unsigned);    // always return true
bool ticking(void);
void USBDeviceInit(void);
bool read_switch(void);
void xpt_init(void), xpt_deinit(), enable_int4(void);
bool xpt(void), ir_init(void);

extern Font font24;
Font *fnt24 = &font24;

char *timer(Cmd,int,int), *calc(Cmd,int,int), *remote(Cmd,int,int);
char *learn(Cmd,int,int);

namespace
{

char *root(Cmd,int,int), *light(Cmd,int,int);

bool dump_flag;
int X, Y, sleep_count;
Device device = &root;

const Device devices[NUM_DEVICE] = {
  &root, &light, &timer, &calc, &remote, &learn
};

char *light(Cmd cmd, int, int) {
  if (cmd == DRAW) tft_cmd(PIXELS_OFF);
  if (cmd == UP) { tft_cmd(NORMAL_ON); menu(0); }
  if (cmd == MAIN) return 0;
  return "LIGHT";
}

void draw_menu(int id, Color c) {
  if (id < 1) return;
  Device dev = devices[id];
  char data[8000];
  gram::set(320, 50, data);
  gram::fill(TEAL);
  gram::fill(BLACK, 28, 2, 264, 46);
  gram::fill(GOLD, 30, 5, 260, 40);
  gram::fill(dev ? c : BUTTER, 38, 10, 244, 30);
  if (dev) gram::print(fnt24, BLUE, (*dev)(NAME, 0, 0), 40, 13);
  tft_write_data(465 - id * 50, data, 8000);
}

char *root(Cmd cmd, int x, int y) {
  static int button = -1;
  if (cmd == DRAW) {
    char data[2400];
    gram::set(320, 15, data);
    gram::fill(TEAL);
    tft_write_data(465, data, 2400);
    for (int i = 1; i < NUM_DEVICE; i++) draw_menu(i, BUTTER);
    tft_write_data(0, data, 2400);
  }
  if (cmd == DOWN) {
    if ((x > 30)&&(x < 290)&&(y > 15)&&(y < 465)) y = (515 - y) / 50;
    else y = 0;
    if (button != y) {
      draw_menu(button, BUTTER);
      draw_menu(button = y, ORANGE);
    }
  }
  if (cmd == UP) {
    Device dev = button > 0 ? devices[button] : 0;
    draw_menu(button, BUTTER);
    button = -1;
    if (dev) menu(dev);
  }
  return "";
}

void dumpScreen(void) {
  char buf[9600];
  dump_flag = false;
  tft_caset(0, 319);
  tft_paset(0, 479);
  xpt_deinit(); sd_init();
  File f;
  if (f.open("ABC     IMG")) {
    tft_bl_off();
    tft_read_start();
    for (int i = 0; i < 48; i++) {
      tft_read_data(buf, 9600);
      f.write(buf, 9600);
    }
    tft_read_end();
    f.close();
    tft_bl_on();
  } else send_cdc("fail to open file\r\n", 19);
  sd_deinit(); xpt_init();
  sleep_count = 0;
}

}//anonymous

void menu(Device d) { (*(device = d ? d : &root))(DRAW, 0, 0); }

int main(void) {
  init();
  USBDeviceInit();
  tft_init();
  xpt_init();
  ir_init();
  (*device)(DRAW, 0, 0);
  while (wait(0)){      // call wait() to transfer control to poll()
    if (ticking()) if (xpt()||!(*device)(MAIN, 0, 0)) sleep_count = 0;
    if (dump_flag) dumpScreen();
    if (sleep_count < SLEEP) continue;
    tft_off();
    enable_int4();
    asm volatile ("wait");
    tft_on();
    sleep_count = 0;
  }
}

void poll(unsigned t) {
  sleep_count++;
  if (!(t & 15)) read_switch();		// read switch every 16 ms
  if (!(*device)(POLL, t, 0)) sleep_count = 0;
  sd_poll(t);
}

void on_switch_change(bool b) {
  if (!b) {
    dump_flag = true;
    toggle_LED();
  }
}

void process_xpt(int x, int y) {
  enum { OX=195, OY=245, TW=3665, TH=3715, SW=320, SH=480 };
  (*device)(DOWN, X = (SW * (x - OX)) / TW, Y = (SH * (y -OY)) / TH);
}

void touch_off(void) { (*device)(UP, X, Y); }
