#ifndef _FAT32_H
#define _FAT32_H

#include "sd.h"

struct __attribute__((__packed__)) Directory {
  char fn[11], attrib, dummy[8];
  int cluster_h, cluster_l, size;
};

class File {
public:
  File(void), File(char*);
  bool open(char*);
  void close(void);
  int write(char*, int), read(char*, int);
private:
  enum { DATA=1, DIR=2 };	// flags
  DataPacket packet;
  int cluster, size, dirSec, sector, pos, flags, secCnt;
  Directory *dir;
  void next_sector(void);
  bool set_cluster(int);
};

#endif //_FAT32_H
