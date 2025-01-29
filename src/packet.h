#ifndef PACKET_H
#define PACKET_H

#define PACKET_DATA_LEGNTH 16
#define PACKET_CRC_LENGTH 1
#define PACKET_LENGTH_LENGTH 1
#define PACKET_LENGTH (PACKET_DATA_LEGNTH - (PACKET_CRC_LENGTH + PACKET_LENGTH_LENGTH))

#define PACKET_RETX_DATA0 0x11
#define PACKET_ACK_DATA0 0x12

// buffer for holding how many buffers
#define PACKET_BUFFER_LENGTH 8

typedef struct {
  uint8_t length;
  uint8_t data[PACKET_DATA_LEGNTH];
  uint8_t crc;
} uart_packet;

typedef enum {
  length_state,
  data_state,
  crc_state
} comms_states;

#ifdef __cplusplus
extern "C" {
#endif

void packet_setup(void);
void write_packet(uart_packet* packet, uint8_t UART_NUM);
void read_packet(uart_packet *packet);
uint8_t calculate_crc(uint8_t* data, uint8_t length);
void packet_update(uint8_t UART_NUM, uart_packet *packet);

#ifdef __cplusplus
}
#endif

#endif