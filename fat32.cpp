/*  Reference:
 *  https://www.pjrc.com/tech/8051/ide/fat32.html
 *
 *  Only support:
 *  FAT32
 *  files in root directory
 *  short file name: eight.3 => "EIGHT   3  "
 *  read FAT (no writing to File Allocation Table)
 *
 *  ie. create a file LARGE enough to hold your data
 *  give it a 8.3 file name eg. EXP1.DAT
 *  and save it in the root directory
 *
 *  notes:
 *  FAT32 seems storing file names in CAPITAL LETTER
 *  even it displays them in lower case.
 */

#include "fat32.h"

bool wait(unsigned);

namespace
{

int secPerClus, rootClus, fatSz32, clusterBegin, fatBegin;

bool read_sector(int s) {
  sd_read(s);
  while (sd_state() == READ) wait(0);
  return sd_state() != ERROR;
}

bool write_sector(int s) {
  sd_write(s);
  while (sd_state() == WRITE) wait(0);
  while (sd_state() == BUSY) wait(0);
  return sd_state() != ERROR;
}

bool fat_ready(DataPacket *packet) {
  if (sd_state() == DISCON) if (sd_connect()) return false;
  if (rootClus) return true;
  sd_buffer(packet);
  if (!read_sector(0)) return false;
  int i;
  for (i = 450; i < 512; i += 16) {
    if (packet->data[i] == 11) break;
    if (packet->data[i] == 12) break;
  }
  if (i > 512) return false;	// not FAT32
  i += 4;
  int partition;
  for (int j = 0; j < 4; j++) ((char*)&partition)[j] = packet->data[i + j];
  if (!read_sector(partition)) return false;
  if (packet->data[17]) return false;	// RootEntCnt = 0 for FAT32
  if (packet->data[18]) return false;	// RootEntCnt = 0 for FAT32
  secPerClus = packet->data[0xd];
  int rsvdSecCnt = packet->data[0xe] + (packet->data[0xf] << 8);
  int *param = (int*)&packet->data[0x24];
  fatBegin = partition + rsvdSecCnt;
  fatSz32 = param[0];
  rootClus = param[2];
  clusterBegin = fatBegin + ((fatSz32 - secPerClus) << 1);
  return true;
}

} // anonymous

File::File(void): cluster(0) { packet.ff = -1; }

File::File(char *fn): File() { open(fn); }

bool File::open(char *fn) {
  if (cluster) return false;
  if (!fat_ready(&packet)) return false;
  sd_buffer(&packet);
  flags = pos = 0;
  set_cluster(rootClus);
  int i = 512;
  while (i == 512) {
    i = 0;
    while (i < 512) {
      if (!packet.data[i]) return false;	// end of directories
      int j = -1;
      while (++j < 11) if (packet.data[i + j] != fn[j]) break;
      if (j == 11) break;
      i += 32;
    }
    if (i == 512) next_sector();
  }
  dirSec = sector;
  dir = (Directory*)&packet.data[i];
  if (dir->attrib & 0x10) return false;	// subdirectory
  int c = (dir->cluster_h << 16) | ((dir->cluster_l >> 16) & 65535);
  if (!set_cluster(c)) return (File*)(cluster = 0);
  size = dir->size;
  return true;
}

bool File::set_cluster(int c) {
  secCnt = secPerClus;
  return read_sector(sector = clusterBegin + secPerClus * (cluster = c));
}

void File::close(void) {
  if (!cluster) return;
  sd_buffer(&packet);
  if (flags & DATA) write_sector(sector);
  if (flags & DIR) {
    read_sector(dirSec);
    dir->size = size;
    write_sector(dirSec);
  }
  cluster = 0;
}

int File::read(char *p, int length) {
  int idx, chunk, cnt = 0;
  if (!cluster) return 0;
  sd_buffer(&packet);
  chunk = size - pos;
  if (length > chunk) length = chunk;	// can't read too much
  while (length) {
    idx = pos & 511;
    chunk = 512 - idx;			// biggest chunk possible
    if (chunk > length) chunk = length;
    for (int i = 0; i < chunk; i++) p[i] = packet.data[idx + i];
    pos += chunk; p += chunk; cnt += chunk; idx += chunk; length -= chunk;
    if (idx == 512) next_sector();
  }
  return cnt;		// return bytes read
}

int File::write(char *p, int length) {
  int idx, chunk, cnt = 0;
  if (!cluster) return 0;
  sd_buffer(&packet);
  while (length) {
    idx = pos & 511;
    chunk = 512 - idx;			// biggest chunk possible
    if (chunk > length) chunk = length;
    for (int i = 0; i < chunk; i++) packet.data[idx + i] = p[i];
    flags |= DATA;			// mark writing
    pos += chunk; p += chunk; cnt += chunk; idx += chunk; length -= chunk;
    if (idx == 512) next_sector();
  }
  if (pos > size) { size = pos; flags |= DIR; }
  return cnt;
}

void File::next_sector(void) {
  if (flags & DATA) write_sector(sector);
  if (--secCnt) read_sector(++sector);
  else {
    read_sector((cluster >> 7) + fatBegin);
    set_cluster(((int*)packet.data)[cluster & 0x7f]);
  }
  flags &= ~DATA;
}
