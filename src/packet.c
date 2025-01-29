#include <string.h>
#include <stdint.h>
#include "packet.h"
#include "uart_defines.h"

uint8_t calculate_crc(uint8_t* data, uint8_t length);
void write_packet(uart_packet* packet, uint8_t UART_NUM);

// inital values for the packets we will use
static uart_packet packet_buffer[PACKET_BUFFER_LENGTH];
static uint32_t write_index = 0, read_index = 0;
static uint32_t packet_mask = PACKET_BUFFER_LENGTH - 1;

static uart_packet retx_packet = {  .length = 0, .data = {0}, .crc = 0, };
static uart_packet ack_packet = {  .length = 0, .data = {0}, .crc = 0, };
static uart_packet last_transmitted_packet = {  .length = 0, .data = {0}, .crc = 0, };
// our packet will always start with the legnth state
static comms_states current_state = length_state;
// counter for how much data bytes we have
static uint8_t data_bytes_counter = 0;

static bool is_packet_retx(const uart_packet* packet_pointer) {
  // compare the contents of the packet and the retx_packet
  return ((memcpy(packet_pointer, &retx_packet, sizeof(uart_packet))) == 0);
}

static bool is_packet_ack(const uart_packet* packet_pointer) {
  // compare the contents of the packet and the ack_packet
  return ((memcpy(packet_pointer, &ack_packet, sizeof(uart_packet))) == 0);
}

void create_single_byte_packet(uart_packet* packet, uint8_t byte) {
  // will take a packet, pad it with ones, give a one length and its unique byte, and lastly calculate its crc
  memset(packet, 0xFF, sizeof(uart_packet));
  packet->length = 1;
  packet->data[0] = byte;
  packet->crc = calculate_crc((uint8_t *) packet, PACKET_LENGTH);
}

void packet_setup(void) {
  // setup the request retransmittion and ack packets
  create_single_byte_packet(&retx_packet, PACKET_RETX_DATA0);
  create_single_byte_packet(&ack_packet, PACKET_ACK_DATA0);
}

void packet_update(uint8_t UART_NUM, uart_packet *packet) {
  // function will create the packet contents
  // by going through each state, reading a byte from the ring buffer,
  // and do the state required computation
  switch (current_state) {

  case length_state: {
    packet->length = uart_read(UART_NUM);
    data_bytes_counter = 0;
    current_state = data_state;
  } break;

  case data_state: {
    while (data_bytes_counter < packet->length) {
      if (!is_data_available_uart(UART_NUM)) { 
        return;
      }
    packet->data[data_bytes_counter++] = uart_read(UART_NUM);
  }
    current_state = crc_state;
  } break;

  case crc_state: {
    // get crc and compute it to check if the data was transmitted correctly
    packet->crc = uart_read(UART_NUM);
    uint8_t computed_crc = calculate_crc((uint8_t *) &packet, PACKET_LENGTH);

    // write special packets for every possible case
    if (packet->crc != computed_crc) {
      write_packet(&retx_packet, UART_NUM);
      current_state = length_state;
      break;
    }

    if (is_packet_retx(&packet)) {
      write_packet(&retx_packet, UART_NUM);
      current_state = length_state;
      break;
    }

    if (is_packet_ack(&packet)) {
      current_state = length_state;
      break;
    }

    // avoid overwriting packets
    uint32_t next_write_index = (write_index + 1) & packet_mask;
    if (next_write_index == read_index) {
      break;

    // save the packet into the buffer, increment the index, and send an ack packet
    memcpy(&packet_buffer[write_index], &packet, sizeof(uart_packet));
    write_index = next_write_index;

    packet_write(&ack_packet, UART_NUM);
    current_state = length_state;

  } break;
  // default state isn't expected to happen at all
    default: {  current_state = length_state; break;  }

    }
  }
}

void write_packet(uart_packet* packet, uint8_t UART_NUM) {
  uart_send(UART_NUM, &packet->length);  // Send length as 1 byte

  for (uint8_t i = 0; i < packet->length; i++) {
    uart_send(UART_NUM, &packet->data[i]);  // Send data byte-by-byte
  }

  uart_send(UART_NUM, &packet->crc);  // Send CRC as 1 byte
  memcpy(&last_transmitted_packet, packet, sizeof(uart_packet));
}

bool is_packet_available(void) {
  return read_index != write_index;
}

void read_packet(uart_packet *packet) {

  if (!is_packet_available()) {
    return;
  }

  memcpy(packet, &packet_buffer[read_index], sizeof(uart_packet));
  read_index = (read_index + 1) & packet_mask;
}

// function will calculate crc 8
uint8_t calculate_crc(uint8_t* data, uint8_t length) {
  uint8_t crc = 0;
  // for every byte, we will xor with data
  for (uint16_t i = 0; i < length; i++)  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      // we will then do some bit calculations to get the crcs
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}