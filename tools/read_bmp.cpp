/*  open BMP file: file must be indexed 8 color, width has to be even
 *  save bitmap to memory: 1 byte per pixel
 */

#include <stdio.h>
#include <stdint.h>

struct __attribute__((__packed__)) Header {
  char id[2];
  int32_t size;
  int32_t res;
  int32_t offset;
}; 

struct __attribute__((__packed__)) DIBhdr {
  int32_t size;
  int32_t width;
  int32_t height;
  short planes;
  short bits;
  int compression, image_size, XPerMeter, YPerMeter, colors;
};

struct ColorTable {
  unsigned char B, G, R, V;
};

int read_bmp_header(char *fn) {
  FILE *fp = NULL;
  Header header;
  DIBhdr dib;
  fp = fopen(fn, "rb");
  if (!fp) {
    printf("cannot open file %s\n", fn);
    return 0;
  }
  fread(&header, sizeof(Header), 1, fp);
  fread(&dib, sizeof(DIBhdr), 1, fp);
  fclose(fp);
  if ((header.id[0] == 'B') && (header.id[1] == 'M'))
    return dib.width * dib.height;
  printf("not a bitmap file\n");
  return 0;
}

bool read_bmp(char *fn, char *buffer, int &width, int &height) {
  FILE *fp = NULL;
  Header hdr;
  DIBhdr dib;
  fp = fopen(fn, "rb");
  if (!fp) {
    printf("cannot open file %s\n", fn);
    return false;
  }
  fread(&hdr, sizeof(Header), 1, fp);
  fread(&dib, sizeof(DIBhdr), 1, fp);
  ColorTable color[dib.colors];
  printf("ID=%c%c  SIZE=%d\n", hdr.id[0], hdr.id[1], hdr.size);
  printf("width=%d  height=%d  bits=%d\n", dib.width, dib.height, dib.bits);
  printf("Colors in Color Table = %d\n", dib.colors);
  if ((dib.bits != 4)&&(dib.bits != 8)) {
    fclose(fp);
    printf("Only support 4-bit and 8-bit color\n");
    return false;
  }
  fseek(fp, dib.size + 14, SEEK_SET);
  fread(color, dib.colors, sizeof(ColorTable), fp);
  for (int i = 0; i < dib.colors; i++) {
    int r = (color[i].R & 0xc0) >> 6;
    int g = (color[i].G & 0xc0) >> 4;
    int b = (color[i].B & 0xc0) >> 2;
    color[i].V = b | g | r;
//    printf("B=%d G=%d R=%d V=%d\n", color[i].B, color[i].G, color[i].R, color[i].V);
  }
  fseek(fp, hdr.offset, SEEK_SET);
  width = dib.width;
  height = dib.height;
  char line[480];
//  int length = width >> 1;	// input = 2 pixel per byte
  int length = dib.bits == 4 ? width >> 1 : width;
  while (length & 3) length++;
  int j_max = dib.bits == 4 ? width >> 1 : width;
  for (int i = 0; i < height; i++) {
    fread(line, length, 1, fp);
    for (int j = 0; j < j_max; j++) {
      if (dib.bits == 4) {
        *buffer++ = color[(line[j] >> 4) & 15].V;
        line[j] &= 15;
      }
      *buffer++ = color[line[j]].V;
    }
  }
  fclose(fp);
  return true;
}
