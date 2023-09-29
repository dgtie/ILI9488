#ifndef _GRAM_H
#define _GRAM_H

enum Color {
  BLACK=0, RED=0111, GREEN=0222, YELLOW=0333,
  BLUE=0444, MAGENTA=0555, AQUA=0666, WHITE=0777,
  MAROON=010, HULK=020, NAVYBLUE=040, BUTTER=0373,
  TEAL=0242, BRESCIANBLUE=0464, PINK=0575, FORBIDDENJUICE=0353,
  GUPPIEGREEN=0262, PURPLE=0141, VIOLET=0454, GOLD=0121,
  PARROT=0565, RADIUM=0232, LIGHTGREEN=0363, ELECTRICBLUE=0676,
  NEONROSE=0151, ORANGE=0131, GREY=0252
};

struct Image { short width, height; unsigned char rle[]; };

struct Font { short width, height, first, mask[]; };

namespace gram
{

void set(int width, int height, char*);
void set(int width, int height);
void fill(Color);
void fill(Color, int x, int y, int w, int h);   // x, w, h must be even 
void load(Image*);
void draw(Image*, int x, int y);		// x must be even
void print(Font*, Color, int c, int x, int y);	// x must be even
void print(Font*, Color, int size, char*, int x, int y);
void print(Font*, Color, char*, int x, int y);
void copy(void);
char *paste(void);

}

#endif //_GRAM_H
