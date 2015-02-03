#ifndef __MISC__H_
#define __MISC__H_

inline int read_packet(int fd, unsigned char* buf, int &size) {
  int res = read(fd,  &size, sizeof(size));
  assert(res >= 0);
  if (res != 0) {
    if (!(res == sizeof(size) && 0 < size && size <= MAX_PACKET_SIZE))
      printf("res=%d, size=%d\n", res, size);
    assert(res == sizeof(size) && 0 < size && size <= MAX_PACKET_SIZE);
    res = read(fd,  buf, size);
    assert(res == size);
  }
  return res;
}
#endif
