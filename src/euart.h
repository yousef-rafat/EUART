#ifndef EUART
#define EUART
#include "uart_defines.h"

#ifdef __cplusplus
extern "C" {
#endif
// functions that user should use
void init_uart(uint32_t baud_rate, struct uart_settings* settings, uint8_t UART_NUM);
void uart_send(uint8_t UART_NUM, const char* data);
void send_packet(uint8_t UART_NUM, const char* str);
uint8_t uart_read(uint8_t UART_NUM);
uart_packet* read_pack(uint8_t UART_NUM); // returns a packet to read

#ifdef __cplusplus
}
#endif

#endif