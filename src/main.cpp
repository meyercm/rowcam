
/*
 * Demo for SSD1306 based 128x64 OLED module using Adafruit SSD1306
 * library (https://github.com/adafruit/Adafruit_SSD1306).
 *
 * See https://github.com/pacodelgado/arduino/wiki/SSD1306-based-OLED-connected-to-Arduino
 * for more information.
 *
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdint.h>

// If using software SPI (the default case):
#define OLED_MOSI  6 //11   //D1
#define OLED_CLK   5 //12   //D0
#define OLED_DC    8 //9
#define OLED_CS    9 //8
#define OLED_RESET 7 //10

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

static const uint8_t IMAGE_WIDTH = 128;
static const uint8_t IMAGE_HEIGHT = 64;
static const uint16_t MSG_SIZE = 256;

static const uint8_t BSP_RESEND_ATTEMPTS = 3;
static const uint8_t BSP_STX = 2;
static const uint8_t BSP_ETX = 3;
static const uint8_t BSP_ACK = 6;
static const uint8_t BSP_NAK = 21;
static const uint8_t BSP_HEADER = 2; // STX + LENGTH

static const uint8_t DISPLAY_ROW = 0;
static const uint8_t DISPLAY_REFRESH = 1;
static const uint8_t ROW_MESSAGE_LENGTH = 17; // header + 128 / 8

static const uint8_t BITMASKS[] = {128, 64, 32, 16, 8, 4, 2, 1};

void setup()   {
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  Serial.begin(19200);
}

static uint8_t in_buffer[MSG_SIZE];
static uint8_t buffer_index = 0;
static uint8_t message_length = 0;

typedef enum {
  RX_IDLE,
  RX_LENGTH_NEXT,
  RX_DATA_NEXT,
  RX_CHECKSUM_NEXT,
  RX_ETX_NEXT,
} RX_FSM;
static RX_FSM rx_fsm = RX_IDLE;

static void send_nak();
static void send_ack();
static void set_fsm(RX_FSM new_state);
static void process_buffer();
static void process_fsm(uint8_t next_byte);
static uint8_t checksum(const uint8_t payload[], uint8_t payload_length);
void loop()
{
  if (Serial.available() > 0) {
    uint8_t next_byte = Serial.read();
    process_fsm(next_byte);
  }
}

static void process_fsm(uint8_t next_byte) {
  switch (rx_fsm) {
    case RX_IDLE:
      if (next_byte == BSP_STX){
        in_buffer[buffer_index++] = next_byte;
        set_fsm(RX_LENGTH_NEXT);
      } else {
        send_nak();
      }
      break;
    case RX_LENGTH_NEXT:
      in_buffer[buffer_index++] = next_byte;
      message_length = next_byte;
      set_fsm(RX_DATA_NEXT);
      break;
    case RX_DATA_NEXT:
      in_buffer[buffer_index++] = next_byte;
      if (buffer_index == BSP_HEADER + message_length) {
        set_fsm(RX_CHECKSUM_NEXT);
      }
      break;
    case RX_CHECKSUM_NEXT:
      if (next_byte == checksum(in_buffer, BSP_HEADER + message_length)) {
        set_fsm(RX_ETX_NEXT);
      } else {
        send_nak();
      }
      break;
    case RX_ETX_NEXT:
      if (next_byte == BSP_ETX) {
        process_buffer();
        send_ack();
      } else {
        send_nak();
      }
      break;
  }
}

static void send_nak(){
  set_fsm(RX_IDLE);
  Serial.write(BSP_NAK);
  Serial.flush();
}
static void send_ack(){
  set_fsm(RX_IDLE);
  Serial.write(BSP_ACK);
  Serial.flush();
}

static void set_fsm(RX_FSM new_state){
  rx_fsm = new_state;
  switch (new_state){
    case RX_IDLE:
      buffer_index = 0;
      message_length = 0;
      break;
    default:
      break;
  }
}

static void process_buffer(){
  switch (in_buffer[2]) {
    case DISPLAY_REFRESH: {
      display.display();
      break;
    }
    case DISPLAY_ROW: {
      for (uint8_t x = 0; x < IMAGE_WIDTH; x++){
        uint8_t next_level = BITMASKS[x % 8] & in_buffer[4 + x / 8];
        display.drawPixel(x, in_buffer[3], next_level);
      }
      //display.display(); //TODO: remove
      break;
    }
    default: {
      break;
    }
  }
}


static uint8_t checksum(const uint8_t payload[], uint8_t payload_length){
  uint8_t checksum = payload[0];
  for (uint8_t i = 1; i < payload_length; i++){
    checksum ^= payload[i];
  }
  return checksum;
}
