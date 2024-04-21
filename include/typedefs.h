#pragma once

#define CC1101_sck 36
#define CC1101_miso 37
#define CC1101_mosi 35
#define CC1101_ss 34
#define CC1101_gdo0 18
#define CC1101_gdo2 33

typedef struct rmt_message_t
{
  rmt_data_t buf[RMT_MEM_NUM_BLOCKS_4 * RMT_SYMBOLS_PER_CHANNEL_BLOCK];
  uint16_t length;
  unsigned long time;
  int64_t delta;
  int rssi;
} rmt_message_t;

typedef struct pwm_message_t
{
  uint8_t buf[RMT_MEM_NUM_BLOCKS_4 * RMT_SYMBOLS_PER_CHANNEL_BLOCK / 8];
  uint8_t length;
} pwm_message_t;