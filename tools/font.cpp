/*  create font bitmap from "ascii.bmp"
 *  font size = 24 (H) x 14 (W)
 *  total 96 characters begins with sp (space)
 *  declaration: const unsigned short font[][21]
 */

#include <stdio.h>
#include <iostream>

using namespace std;

bool read_bmp(char *fn, char *buffer, int &width, int &height);
int read_bmp_header(char *fn);

int width; int height;
int font_width, font_height;

void ascii_print(char *buffer) {
  for (int r = 0; r < font_height; r++) {
    for (int c = 0; c < font_width; c++) cout << (int)*buffer++;
    cout << endl;
  }
}

void ascii(char *image, int x, int y) {
  int bitmap_size = (font_width * font_height) >> 4;
  char *buffer = (char*)malloc(font_width * font_height);
  char *ptr = buffer;
  unsigned short *bitmap = (unsigned short*)malloc(bitmap_size << 1);
  image += x + (width * y);
  for (int r = 0; r < font_height; r++) {
    for (int c = 0; c < font_width; c++) *ptr++ = image[c];
    image += width;
  }
//  ascii_print(buffer);
  ptr = buffer;
  for (int i = 0; i < bitmap_size; i++) {
    if (i) cout << ", ";
    bitmap[i] = 0;
    for (int j = 0; j < 16; j++) {
      bitmap[i] <<= 1;
      if (*ptr++) bitmap[i]++;
    }
    cout << bitmap[i];
  }
  cout << endl;
  ptr = buffer;
  for (int i = 0; i < bitmap_size; i++) {
    int bm = bitmap[i];
    for (int j = 0; j < 16; j++) {
      *ptr++ = bm & 0x8000 ? 0  : 1 ;
      bm <<= 1;
    }
  }
//  ascii_print(buffer);
  free(bitmap);
  free(buffer);
}

int main(int argc, char* argv[]) {
  int x, y;
  int line_space;
  if (argc == 5) {
    font_width = atoi(argv[2]);
    font_height = atoi(argv[3]);
    line_space = atoi(argv[4]);
    printf("font width = %d, font height = %d, line space = %d\n",
            font_width, font_height, line_space);
    int file_size = read_bmp_header(argv[1]);
    if (!file_size) return 0;
    char *buffer = (char*)malloc(file_size);
    if (buffer == NULL) {
      printf("not enough memory!\n");
      exit(1);
    }
    read_bmp(argv[1], buffer, width, height);
    x = 0; y = height - font_height;
    int index = 0;
    printf("const struct { short width, height, first, mask[arraySize]; } font_name = { %d, %d, 32, {\n", font_width, font_height);
    while (1) {
//      printf("%2d: x = %3d, y = %3d\n", index, x, y);
      if (index++) cout << ", ";
      ascii(buffer, x, y);
      x += font_width;
      if (x >= width) {
        x = 0;
        y -= font_height + line_space;
        if (y < 0) break;
      }
    }
    cout << "}\n};" << endl;
    printf("arraySize = %d\n", index * ((font_width * font_height) >> 4));
    free(buffer);
  } else printf("Please provide a BMP file, font width, font height, line space\n./font font24.bmp 14 24 5\n");
  return 0;
}
