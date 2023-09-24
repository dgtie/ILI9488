#include <stdio.h>
#include <iostream>

using namespace std;

bool read_bmp(char *fn, char *buffer, int &width, int &height);

void print(unsigned char *buffer, int width, int height) {
  char hex[] = "0123456789ABCDEF";
  for (int y = height; y != 0; y--) {
    for (int x = 0; x < width; x++)
      cout << hex[buffer[(y - 1) * width + x]];
    cout << endl;
  }
}

char swap(char c, int sw) {
  if (sw) {
    int hi = c & 0x3f;
    int lo = c & 7;
    hi >>= 3; lo <<= 3;
    c &= 0xc0;
    c |= hi | lo;
  }
  return c;
}
//void two2one(char *d, char *s, int finalSize) {
void two2one(char *d, char *s, int width, int height) {
  width >>= 1;
  char color[] = {
    (char)0b11000000,	// 000000 BLACK
                   0,	// 000001
    (char)0b11000001,	// 000010 MAROON
    (char)0b11001001,	// 000011 RED
                   0,	// 000100
                   0,   // 000101
                   0,	// 000110
                   0,   // 000111
    (char)0b11011011,	// 001000 HULK
                   0,	// 001001
    (char)0b11010001,	// 001010 HEART GOLD
    (char)0b11011001,	// 001011 ORANGE JUICE
    (char)0b11010010,	// 001100 GREEN
                   0,	// 001101
    (char)0b11010011,	// 001110 RADIUM
    (char)0b11011011,	// 001111 YELLOW
                   0,	// 010000
                   0,	// 010001
                   0,	// 010010
                   0,	// 010011
                   0,	// 010100
    (char)0b00111111,	// 010101 (transparent)
                   0,	// 010110
                   0,	// 010111
                   0,	// 011000
                   0,	// 011001
                   0,	// 011010
                   0,	// 011011
                   0,	// 011100
                   0,	// 011101
                   0,	// 011110
                   0,	// 011111
    (char)0b11100000,	// 100000 NAVY BLUE
                   0,	// 100001
    (char)0b11100001,	// 100010 PURPLE
    (char)0b11101001,	// 100011 NEON ROSE
                   0,	// 100100
                   0,	// 100101
                   0,	// 100110
                   0,	// 100111
    (char)0b11100010,	// 101000 TEAL
                   0,	// 101001
    (char)0b11100011,	// 101010 GREY
    (char)0b11101011,	// 101011 FORBIDDEN FRUIT
    (char)0b11110010,	// 101100 GUPPIE GREEN
                   0,	// 101101
    (char)0b11110011,	// 101110 LIGHT GREEN
    (char)0b11111011,	// 101111 BUTTER
    (char)0b11100100,	// 110000 BLUE
                   0,	// 110001
    (char)0b11100101,	// 110010 VIOLENT VIOLET
    (char)0b11101101,	// 110011 MAGENTA
                   0,	// 110100
                   0,	// 110101
                   0,	// 110110
                   0,	// 110111
    (char)0b11100110,	// 111000 BRESCIAN BLUE
                   0,	// 111001
    (char)0b11110101,	// 111010 BLUE PARTY PARROT
    (char)0b11111101,	// 111011 HOTTEST OF PINKS
    (char)0b11110110,	// 111100 AQUA
                   0,	// 111101
    (char)0b11110111,	// 111110 ELECTRIC BLUE 
    (char)0b11111111	// 111111 WHITE
  };
  char c;
  int toggle = 0;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      c = swap(color[*s++], toggle) & 0b10111000;
      *d++ = c | (swap(color[*s++], toggle) & 0b01000111);
    }
    toggle ^= 1;
  }
}

void one2two(char *d, char *s, int initialSize) {
  char c;
  for (int i = 0; i < initialSize; i++) {
    c = *s++;
    *d++ = c & 0x80 ? (c >> 3) & 7 : 15;
    *d++ = c & 0x40 ? c & 7 : 15;
  }
}

void rl_compress(char *d, char *s, int size) {
  char last = 62, c;
  int count = 0;
  for (int i = 0; i < size; i++) {
    c = *s++;
    if (c == last) count++;
    else {
      if (count) {
        if (count < 3) while (count--) *d++ = last;
        else {
          *d++ = count >> 8;
          *d++ = count & 255;
        }
      }
      count = 0;
      *d++ = last = c;
    }
  }
  if (count) {
    *d++ = count >> 8;
    *d++ = count & 255;
  }
  *d = 62;
}

void rl_decode(char *d, unsigned char *s) {
  unsigned char c = *s++;
  int count = 1;
  while (count) {
    *d++ = c;
    if (!--count) {
      if (*s != 62) {
        if (*s > 62) { c = *s++; count++; }
        else {
          count = *s++ << 8;
          count += *s++;
        }
      }
    }
  }
}

int main(int argc, char* argv[]) {
  int width; int height;	// to read by read_bmp
  char buffer[76800];
  char buffer1[38400];
  if (argc > 1) {
    if (!read_bmp(argv[1], buffer, width, height)) exit(1);
//    print((unsigned char*)buffer, width, height);
//    two2one(buffer1, buffer, (width >> 1) * height);
    two2one(buffer1, buffer, width, height);
    rl_compress(buffer, buffer1, (width >> 1) * height);

//    rl_decode(buffer1, (unsigned char*)buffer);
//    one2two(buffer, buffer1, (width >> 1) * height);
//    print((unsigned char*)buffer, width, height);

    cout << "const struct { short width, height; unsigned char rle[arraySize]; }" << endl;
    cout << "imageName = { " << width << ", " << height << ", " << endl;
    cout << "{ ";
    unsigned char *s = (unsigned char*)buffer;
    int count = 1;
    while (*s != 62) {
      cout << (int)*s++ << ", ";
      if (!(count++ & 15)) cout << endl;
    }
    cout << (int)*s << " }" << endl;
    cout << "};" << endl;
    cout << "arraySize = " << count << endl;
  } else printf("Please provide a BMP file\n");
  return 0;
}
