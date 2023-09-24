static unsigned mod10(unsigned &i) {
  unsigned r = i;
  i /= 10;
  return (r -= i * 10);
}

void int2str(unsigned i, char *p) {
  if (!i) *p = '0';
  while (i) *p-- = mod10(i) + '0';
}

void int2hex(unsigned i, char *p) {
  const char hex[] = "0123456789ABCDEF";
  for (int j = 0; j < 4; j++) {
    int k = i & 15;
    i >>= 4;
    *p-- = hex[k];
  }
}
