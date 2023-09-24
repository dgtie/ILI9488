#ifndef _SD_H
#define _SD_H

enum STATE { DISCON, READY, ERROR, READ, WRITE, BUSY };

union DataPacket {
  struct { int ff; char data[512]; short checksum; };
  struct __attribute__((__packed__)) { char idle[3], token; };
};

STATE sd_state(void);
void sd_init(void), sd_poll(unsigned), sd_deinit(void);
int sd_connect(void), sd_write(int);
bool sd_read(int);
void sd_buffer(DataPacket*);

#endif // _SD_H
