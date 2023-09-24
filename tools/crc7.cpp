#include <iostream>

using namespace std;

unsigned char cmd[] = { 64, 0, 0, 0, 0, 0 };

int main(int argc, char* argv[]) {
  unsigned char acc = 0, polynomial = 0x12;
  for (int i = 0; i < 6; i++) {
    unsigned char c = cmd[i];
    for (int j = 0; j < 8; j++) {
      bool b = acc & 0x80;
      acc <<= 1;
      if (c & 0x80) acc++;
      c <<= 1;
      if (b) acc ^= polynomial;
    }
  }
  cout << hex << (int)acc << endl;
  return 0;
}
