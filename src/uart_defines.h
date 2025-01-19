#ifndef UART_DEFINES
#define UART_DEFINES
// Main Offests
#define UART0_BASE 0x3FF40000
#define UART1_BASE 0x3FF50000
#define UART2_BASE 0x3FF6E000

// interrupt handling for UART0
#define UART0_INTERRUPT_ENABLE (UART0_BASE + 0x0C)
#define UART1_INTERRUPT_ENABLE (UART1_BASE + 0x0C)
#define UART2_INTERRUPT_ENABLE (UART2_BASE + 0x0C)

#define UART0_INTERRUPT_DISABLE (UART0_BASE + 0x10)
#define UART1_INTERRUPT_DISABLE (UART1_BASE + 0x10)
#define UART2_INTERRUPT_DISABLE (UART2_BASE + 0x10)

// configurations for UART0
#define UART0_CONFIG0 (UART0_BASE + 0x20)
#define UART1_CONFIG0 (UART1_BASE + 0x20)
#define UART2_CONFIG0 (UART2_BASE + 0x20)

#define UART0_CONFIG1 (UART0_BASE + 0x24)
#define UART1_CONFIG1 (UART1_BASE + 0x24)
#define UART2_CONFIG1 (UART2_BASE + 0x24)

// General UART handling
#define UART0_CLK_DIVIDER (UART0_BASE + 0x14)
#define UART1_CLK_DIVIDER (UART1_BASE + 0x14)
#define UART2_CLK_DIVIDER (UART2_BASE + 0x14)

// fifo uarts enable
#define UART0_FIFO 0x3FF40000
#define UART1_FIFO 0x3FF50000
#define UART2_FIFO 0x3FF6E000

// UART interrupt status registers
#define UART0_INT_ST (UART0_BASE + 0x08)
#define UART1_INT_ST (UART1_BASE + 0x08)
#define UART2_INT_ST (UART2_BASE + 0x08)

// Register handling
#define READ_REG(addr) (*(volatile uint32_t*)(addr))
#define WRITE_REG(addr, value) (*(volatile uint32_t*)(addr) = (value))

#include <stdbool.h>
#include <stdint.h>

struct uart_settings {
  bool hardware_flow_control;
  bool software_flow_control;
  bool parity;
  uint8_t data_bits;
  uint8_t stop_bits;
  bool invert_pins;
  uint8_t ring_buffer_size;
  bool parity_check_mode;
  bool loopback_mode;
  uint32_t esp_clock_freq;
};

typedef struct {
  uint8_t UART_NUM;
  char* data;
  uint16_t length;
} UART_INPUT;

#ifdef __cplusplus
extern "C" {
#endif

void init_uart(uint32_t baud_rate, struct uart_settings* settings, uint8_t UART_NUM);
void uart_send(uint8_t UART_NUM, char* data);
void uart_read(uint8_t UART_NUM);
#ifdef __cplusplus
}
#endif

#endif