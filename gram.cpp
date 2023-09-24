#include "gram.h"

class RunLength {
public:
  RunLength(Image *img): count(1), last(0)
  { data = img->rle; width = img->width >> 1; }
  int get_row(int *row) { 	// return number of bytes read
    if (last == 62) return 0;
    for (int i = 0; i < width; i++) row[i] = get();
    return width;
  }
private:
  unsigned char *data;
  int width, last, count;
  int get(void) {
    if (last == 62) return last;	// 62 = end of data
    if (!--count) {
      if (*data < 62) count = (*data++ << 8) + *data++;
      else { last = *data++; count++; }
    }
    return last;
  }
};

class Bitmap {
public:
  Bitmap(short *bmp, Color c): bitmap(bmp), bit(0x8000), toggle(0) {
    ch[0] = c & 0b111000; ch[0] ^= 0b10111000;
    cl[0] = c & 0b111; cl[0] ^= 0b1000111;
    c = (Color)(c >> 3);
    ch[1] = c & 0b111000; ch[1] ^= 0b10111000;
    cl[1] = c & 0b111; cl[1] ^= 0b1000111;
  } 
  void get_row(int *row, int size) {
    for (int i = 0; i < size; i++) row[i] = get();
    toggle ^= 1;
  }
private:
  int ch[2], cl[2], bit, toggle;
  short *bitmap;
  int get(void) {
    int c = 0b111111;
    if (!(*bitmap & bit)) c ^= ch[toggle]; bit >>= 1;
    if (!(*bitmap & bit)) c ^= cl[toggle]; bit >>= 1;
    if (!bit) { bit = 0x8000; bitmap++; }
    return c;
  }
};

namespace
{

int width, height;
char data[25600];

} //anonymous

namespace gram
{

char *set(int w, int h) { width = w; height = h; return data; }

char *fill(Color c) {
  int color[2];
  color[0] = c & 077;
  color[1] = c >> 3;
  int n = width >> 1;
  char *ptr = data;
  for (int i = 0; i < height; i++)
    for (int j = 0; j < n; j++)
      *ptr++ = color[i & 1];
  return data;
}

char *fill(Color c, int x, int y, int w, int h) {
  int color[2];
  color[0] = c & 077;
  color[1] = c >> 3;
  int n = w >> 1;
  char *ptr = &data[(y * width + x) >> 1];
  for (int i = y; i < (y + h); i++) {
    for (int j = 0; j < n; j++)
      ptr[j] = color[i & 1];
    ptr += width >> 1;
  }
  return data;
}

char *load(Image *img) {
  set(img->width, img->height);
  return draw(img, 0, 0);
}

char *draw(Image *img, int x, int y) {
  RunLength rle(img);
  int imgWidth = img->width >> 1;
  int buffer[imgWidth];
  char *d = &data[(y * width + x) >> 1];
  for (int i = 0; i < img->height; i++) {
    rle.get_row(buffer);
    for (int j = 0; j < imgWidth; j++) {
      int a = buffer[j];
      if (a & 0x80) d[j] |= 0b111000; 
      if (a & 0x40) d[j] |= 0b111; 
      d[j] &= a;
    }
    d += (width >> 1);
  }
  return data;
}

char *print(Font *fnt, Color c, int ch, int x, int y) {
  int i = (ch - fnt->first) * ((fnt->width * fnt->height) >> 4);
  Bitmap bitmap(&fnt->mask[i], c);
  int fontWidth = fnt->width >> 1;
  int buffer[fontWidth];
  char *d = &data[(y * width + x) >> 1];
  for (i = 0; i < fnt->height; i++) {
    bitmap.get_row(buffer, fontWidth);
    for (int j = 0; j < fontWidth; j++) {
      int a = buffer[j];
      if (a & 0x80) d[j] |= 0b111000;
      if (a & 0x40) d[j] |= 0b111;
      d[j] &= a;
    }
    d += (width >> 1);
  }
  return data;
}

char *print(Font *fnt, Color color, int size, char *s, int x, int y) {
  int i = 0;
  while (i < size) {
    char c = s[i++];
    if (c != 32) print(fnt, color, c, x, y);
    x += fnt->width;
  }
  return data;
}

char *print(Font *fnt, Color color, char *s, int x, int y) {
  while (*s) {
    char c = *s++;
    if (c != 32) print(fnt, color, c, x, y);
    x += fnt->width;
  }
  return data;
}

void copy(void) {
  int size = (width >> 1) * height;
  char *dest = &data[size];
  for (int i = 0; i < size; i++) dest[i] = data[i];
}

char *paste(void) {
  int size = (width >> 1) * height;
  char *src = &data[size];
  for (int i = 0; i < size; i++) data[i] = src[i];
  return data;
}

} //namespace gram
