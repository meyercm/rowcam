#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdint.h>

static const uint8_t IMAGE_WIDTH = 128;
static const uint8_t IMAGE_HEIGHT = 64;
static const uint8_t PIXEL_SIZE = 3;
static const uint16_t MSG_SIZE = 256;

static const uint8_t BSP_RESEND_ATTEMPTS = 3;
static const uint8_t BSP_STX = 2;
static const uint8_t BSP_ETX = 3;
static const uint8_t BSP_ACK = 6;
static const uint8_t BSP_NAK = 21;

static const uint8_t DISPLAY_ROW = 0;
static const uint8_t DISPLAY_REFRESH = 1;
static const uint8_t ROW_MESSAGE_LENGTH = 18; // MSG_ID + ROW_NUM + (16 = 128 / 8)

static const uint8_t BITMASKS[] = {128, 64, 32, 16, 8, 4, 2, 1};

static const uint8_t STDOUT = 1;
static const uint8_t STDIN = 0;


static size_t craft_message(uint8_t* pixels, uint8_t* buffer, uint8_t row_num);
static void send(uint8_t* message, size_t length);
static void wait_for_ack(uint8_t* message, size_t length, uint8_t attempts);
static void clear_stdin();
static void send_refresh();
static uint8_t checksum(const uint8_t payload[], uint8_t payload_length);

int main(int argc, char* argv[]){
  // expect `./transmitter filename.rgb > /dev/ttyAMA0 < /dev/ttyAMA0`
  assert(argc == 2);
  setbuf(stdout, NULL);
  setbuf(stdin, NULL);

  struct stat st;
  stat(argv[1], &st);
  assert(st.st_size == IMAGE_WIDTH * IMAGE_HEIGHT * PIXEL_SIZE);

  FILE* f = fopen(argv[1], "r");
  uint8_t in_buffer[IMAGE_WIDTH * PIXEL_SIZE];
  uint8_t out_buffer[MSG_SIZE];
  // flush stdin in case the TTY was talking back to a different instance
  clear_stdin();

  for (uint8_t y = 0; y < IMAGE_HEIGHT; y++){
    fprintf(stderr, "starting row %d\n", y+1);
    // For each row, read the pixels, make a row message, send it, wait for an ack
    fread(&in_buffer, 1, IMAGE_WIDTH * PIXEL_SIZE, f);
    size_t length = craft_message(in_buffer, out_buffer, y);
    send(out_buffer, length);
    wait_for_ack(out_buffer, length, BSP_RESEND_ATTEMPTS);
  }
  // sent the whole image, instruct the display to refresh
  fprintf(stderr, "sending refresh cmd\n");
  send_refresh();
  // clean up
  fclose(f);
  fprintf(stderr, "done OK\n");
  return 0;
}

static size_t craft_message(uint8_t* pixels, uint8_t* out_buffer, uint8_t row_num) {
  uint8_t row_byte_size = IMAGE_WIDTH/8;
  out_buffer[0] = BSP_STX;
  out_buffer[1] = ROW_MESSAGE_LENGTH;
  out_buffer[2] = DISPLAY_ROW;
  out_buffer[3] = row_num;
  for (uint8_t x = 0; x < row_byte_size; x++){
    uint8_t next_byte = 0;
    for (int i = 0; i < 8; i++){
      next_byte |= (BITMASKS[i] & pixels[(x*8 + i)*3]);
    }
    out_buffer[4 + x] = next_byte;
  }
  out_buffer[4 + row_byte_size] = checksum(out_buffer, 4 + row_byte_size);
  out_buffer[4 + row_byte_size + 1] = BSP_ETX;
  return 4 + row_byte_size + 2;
}

static void send(uint8_t* buffer, size_t length) {
  write(STDOUT, buffer, length);
}
static void send_refresh(){
  uint8_t buffer[] = {BSP_STX, 1, DISPLAY_REFRESH, 0, BSP_ETX};
  buffer[3] = checksum(buffer, 3);
  send(buffer, 5);
  wait_for_ack(buffer, 5, BSP_RESEND_ATTEMPTS);
}

static void wait_for_ack(uint8_t* out_buffer, size_t length, uint8_t attempts){
  assert(attempts > 0); // exit if we use up our attempts
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  struct timeval tv;
  tv.tv_sec = 10;
  tv.tv_usec = 150 * 1000; // 150 ms
  int result = select(1, &rfds, NULL, NULL, &tv);
  assert(result == 1);
  uint8_t buffer[1];
  read(STDIN, &buffer, 1);
  if (buffer[0] == BSP_ACK) {
    fprintf(stderr, "Got ACK\n");
    return;
  } else if (buffer[0] == BSP_NAK){
    fprintf(stderr, "Got NAK\n");
    send(out_buffer, length);
    wait_for_ack(out_buffer, length, attempts - 1);
  }
}
// read from stdin until it is not ready- flushing away any lost input from a
// previous run
static void clear_stdin() {
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 1 * 1000; // 1 ms
  while(select(1, &rfds, NULL, NULL, &tv)) { (void) false;}
}

static uint8_t checksum(const uint8_t payload[], uint8_t payload_length){
  uint8_t checksum = payload[0];
  for (uint8_t i = 1; i < payload_length; i++){
    checksum ^= payload[i];
  }
  return checksum;
}
