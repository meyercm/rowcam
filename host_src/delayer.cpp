#include <time.h>
#include <unistd.h>
#include <stdint.h>

static const uint8_t STDOUT = 1;
static const uint8_t STDIN = 0;

// For 19200: 1 / 19200 = 52us per bit, so 520us per byte
static const uint32_t SLEEP_NS = 520000;

void send(uint8_t byte) {
  uint8_t out_buffer[1];
  out_buffer[0] = byte;
  write(STDOUT, &out_buffer, 1);
}

int main() {
  uint8_t in_buffer[1];
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = SLEEP_NS;
  while(read(STDIN, &in_buffer, 1) > 0)
  {
    send(in_buffer[0]);
    nanosleep(&ts, NULL);
  }
  return 0;
}
