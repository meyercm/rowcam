#include <unistd.h>
#include <stdint.h>

static const uint8_t STDOUT = 1;
static const uint8_t STDIN = 0;

void send(uint8_t byte) {
  uint8_t out_buffer[1];
  out_buffer[0] = byte;
  write(STDOUT, &out_buffer, 1);
}

int main() {
  uint8_t in_buffer[3];
  uint8_t current_value = 0;
  uint8_t count = 0;
  while(read(STDIN, &in_buffer, 3) > 0)
  {
    // regular rle
    if(in_buffer[0] == current_value){
      count++;
    } else {
      send(count);
      current_value = in_buffer[0];
      count = 1;
    }
    //deal with size constraint
    if (count == 255) {
      send(254); //send the max size item
      send(0); //send zero-count for the opposing item
      count = 1; //keep the leftover
    }
  }
  //send closing byte
  send(255);
  return 0;
}
