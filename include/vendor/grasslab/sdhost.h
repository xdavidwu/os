#ifndef SDHOST_H
#define SDHOST_H

extern void sd_init();
extern void readblock(int block_idx, void *buf);
extern void writeblock(int block_idx, void *buf);

#endif
