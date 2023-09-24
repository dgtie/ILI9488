//  PR2 = 999;			// 40 kHz (1000-40, 1052-38, 1111-36)
#define CARRIER_40 0
#define CARRIER_38 52
#define CARRIER_36 111

namespace
{

int toggle;

unsigned char *rc6_bits(unsigned char *p, unsigned c) {
  const unsigned char bit2[] =
    { 0x58, 0x68, (unsigned char)0x98, (unsigned char)0xa8 };
  unsigned d = c >> 6;
  *p++ = bit2[d & 3];
  d = c >> 4;
  *p++ = bit2[d & 3];
  d = c >> 2;
  *p++ = bit2[d & 3];
  *p++ = bit2[c & 3];
  return p;
}

} //anonymous

void rc6(unsigned char *p, unsigned a, unsigned c) {
  *p++ = CARRIER_36;
  *p++ = 16;
  *p++ = 0xfe;
  *p++ = 0x28;
  *p++ = 0x56;
  *p++ = (toggle ^= 1) ? 0xC8 : 0x38;
  p = rc6_bits(p, a);
  p = rc6_bits(p, c);
  *p++ = *p++ = 101;
  *p++ = 0x80;
  *p = 14;
}
