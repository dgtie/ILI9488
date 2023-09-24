#ifndef _TFT_H
#define _TFT_H

#define READ_PIXEL_FORMAT 0x0C
#define SLEEP_IN     0x10
#define SLEEP_OUT    0x11
#define NORMAL_ON    0x13
#define PIXELS_OFF   0x22
#define DISPLAY_OFF  0x28
#define DISPLAY_ON   0x29
#define CASET        0x2A
#define PASET        0x2B
#define MEMORY_WRITE 0x2C
#define MEMORY_READ  0x2E
#define MEMORY_ACCESS_CONTROL 0x36
#define WRITE_PIXEL_FORMAT 0x3A

void tft_cmd(char);
int tft_cmd(char command, char data);
void tft_on(void);
void tft_off(void);
void tft_write_data(char*, int);		// only transfer data
void tft_write_data(int page, char*, int);	// with cmd @ begin and end
void tft_read_start(void);
void tft_read_data(char*, int);
void tft_read_end(void);
void tft_caset(short start, short end);
void tft_paset(short start, short end);
void tft_init(void);
void tft_bl_on(void), tft_bl_off(void);

#endif //_TFT_H
