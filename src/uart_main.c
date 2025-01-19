#include <stdint.h>
#include "uart_defines.h"
#include "ring_buffer.h"
#include "esp_intr_alloc.h"
#include "esp_attr.h"
// the uart driver is for adding the interrupt for the esp. The ETS_UARTS.
#include "driver/uart.h"
#include "esp_log.h"
#include <stdio.h>

const uint32_t UARTS_FIFO[] = {  UART0_FIFO, UART1_FIFO, UART2_FIFO  };
const uint32_t UARTS_DIS_INT[] = { UART0_INTERRUPT_DISABLE, UART1_INTERRUPT_DISABLE, UART2_INTERRUPT_DISABLE  };
const uint32_t ETS_UARTS[] = {  ETS_UART0_INTR_SOURCE, ETS_UART1_INTR_SOURCE, ETS_UART2_INTR_SOURCE  };
const uint32_t UARTS_ENB_INT[] = {  UART0_INTERRUPT_ENABLE, UART1_INTERRUPT_ENABLE, UART2_INTERRUPT_ENABLE  };
const uint32_t UARTS_INT_ST[] = {  UART0_INT_ST, UART1_INT_ST, UART2_INT_ST  };

// to avoid dynamic allocation hassles, we will make the buffer presist throught the whole program
static RingBuffer *ring_buffer;
static uint8_t *buffer = NULL;

bool is_data_available_uart(uint8_t UART_NUM) {
  // checks if the first bit is there in the UART Interrupt register
  // the bit is set if data is available
  return (READ_REG(UARTS_INT_ST[UART_NUM]) & (1 << 0)) != 0;
}

static UART_INPUT uart_input_ = { .UART_NUM = 0, .data = NULL, .length = 0 };

void uart_send(uint8_t UART_NUM, char* data) {
  // change the values of data sent
  uart_input_.UART_NUM = UART_NUM;
  uart_input_.length = strlen(data);
  uart_input_.data = data;
  
  // clean the register and trigger the TX FIFO WRITE interrupt
  WRITE_REG(UARTS_ENB_INT[UART_NUM], 0);
  WRITE_REG(UARTS_ENB_INT[UART_NUM], (1 << 1));
}

void uart_read(uint8_t UART_NUM) {
  // trigger the FIFO RX interrupt
  WRITE_REG(UARTS_ENB_INT[UART_NUM], (1 << 0));
  uint8_t* data;
  read_ring_buffer(ring_buffer, data);
  printf("%s", (char*)data);
}

void IRAM_ATTR uart_isr(void* arg) {
  // For esp32, there's only one interrupt for each UART. We will have both read and write interrupts here
  UART_INPUT* uart_input = (UART_INPUT*)arg;
  // get uart number from arg
  uint8_t UART_NUM = uart_input->UART_NUM;
  // read the interrupt status to see if its a write or read
  uint32_t status = READ_REG(UARTS_INT_ST[UART_NUM]);

  // if RX FIFO FUll interrupt is enabled, continue
  if (status & (1 << 0)) {

    while (is_data_available_uart(UART_NUM)) {
      // read data in the register and clear the reserved bits
      uint8_t data = READ_REG(UARTS_FIFO[UART_NUM]) & 0xFF;
      // write the data into the ring buffer so we can read it later
      write_ring_buffer(ring_buffer, data);
    }
  }

  // if TX FIFO EMPTY interrupt is enabled, continue
  if (status & (1 << 1)) {
    // get the data from uart_input
    char* data = uart_input_.data;

    // write user data using UART
    // check if the interrupt is enabled
    while (uart_input_.length > 0 && (READ_REG(UARTS_ENB_INT[UART_NUM]) & (1 << 1))) {
      // go through the data and write it the FIFO register
      WRITE_REG(UARTS_FIFO[UART_NUM], *data); // must derefrence the data before sending
      uart_input_.length--;
      data++;
    }

    if (uart_input_.length == 0) {
      WRITE_REG(UARTS_DIS_INT[UART_NUM], (1 << 1));
    }
  }
  // clear interrupts so they won't retrigger
  WRITE_REG(UARTS_DIS_INT[UART_NUM], 1 >> 0);
  WRITE_REG(UARTS_DIS_INT[UART_NUM], (1 << 1));
}

void init_uart(uint32_t baud_rate, struct uart_settings* settings, uint8_t UART_NUM) {

  if (settings == NULL) {
    printf("Error: UART settings cannot be NULL.");
    return;
  }

  // check if the clock was set by the user or not. Otherwise defaults to 80MH
  uint32_t clock_freq = settings->esp_clock_freq ? settings->esp_clock_freq : (80 * 1000000);
  // the division sets the UART clock rate to match the baud rate of transmission
  uint32_t clk_divider = clock_freq / baud_rate; // divides the clock with the baud rate

  // if user didn't specify a ring buffer size, deafult to 32
  settings->ring_buffer_size = settings->ring_buffer_size ? settings->ring_buffer_size : DEFAULT_RING_BUFFER_SIZE;

  // if buffer size is not power of 2, return
  if ((settings->ring_buffer_size - 1) & settings->ring_buffer_size != 0) {
    printf("Buffer size must be a power of 2");
    return;
  }

  // dynamically allocate the memory for the static variable
  buffer = (uint8_t*) malloc(settings->ring_buffer_size);
  if (buffer == NULL) {
      printf("Failed to allocate memory for UART buffer.\n");
      free(buffer);
      return;
  }

  // allocate memory for the ring buffer
  ring_buffer = (RingBuffer*) malloc(sizeof(RingBuffer));
  if (ring_buffer == NULL) {
      printf("Failed to allocate memory for Ring Buffer.\n");
      free(ring_buffer);
      return;
  }

  setup_ring_buffer(ring_buffer, buffer, settings->ring_buffer_size);

  uint32_t config_uart = 0;
  uint32_t config_1_uart = 0;
  uint32_t interrupts = 0;
  // whether to enable or diable the parity bit
  if (settings->parity) {
    // the parity bit is the second bit of the config uart0 register
    config_uart |=  (1 << 1);
    // set the mode of the parity bit
    // 0: even, 1: odd.
    config_uart |= (0 << settings->parity_check_mode);
  }

  // enable the UART_RXFIFO_FULL_INT_ENA interrupt
  interrupts |= (1 << 0);
  // enable the UART_TXFIFO_EMPTY interrupt
  interrupts |= (1 << 1);

  if (settings->software_flow_control) {
    // enables software flow control
    // if the reciever is not read to recieve data, it will tell the transmitter
    // using the RTS signal to pause the sending of data
    config_1_uart |= (23 << 1);
    settings->hardware_flow_control = false; // disable hardware flow to avoid conflicts
  }

  if (settings->loopback_mode) {
    // setting the loopback mode if specified
    config_uart |= (14 << 1);
  }
  // shifting 8 bits for data
  // from 0010 to 1000 
  config_uart |= (3 << 2);
  // the stop bit
  config_uart |= (1 << 4);

  if (settings->hardware_flow_control) {
    config_uart |= (1 << 6);
    // disable software flow control to avoid conflicts
    config_1_uart |= (23 << 0);
  }

  if (UART_NUM > 2) {  printf("UART number can't be larger than 2."); return;  };

  // add ISRs to interrupts
  // choose which uart to add the ISR for
  // put the ISR into the instruction ram
  esp_err_t err = esp_intr_alloc(ETS_UARTS[UART_NUM], ESP_INTR_FLAG_IRAM, uart_isr, (void*) &uart_input_, NULL);
  if (err != ESP_OK) {
    printf("Failed to allocate UART interrupt: %d\n", err);
    return;
  }

  // it's important to add the ISR before we modify the interrupt register
  // this is important for handling interrupts that will fire immediatly
  switch (UART_NUM) {
    case 0:
        WRITE_REG(UART0_CONFIG0, config_uart);
        WRITE_REG(UART0_CONFIG1, config_1_uart);
        WRITE_REG(UART0_CLK_DIVIDER, clk_divider);
        WRITE_REG(UART0_INTERRUPT_ENABLE, interrupts);
        break;

    case 1:
        WRITE_REG(UART1_CONFIG0, config_uart);
        WRITE_REG(UART1_CONFIG1, config_1_uart);
        WRITE_REG(UART1_CLK_DIVIDER, clk_divider);
        WRITE_REG(UART1_INTERRUPT_ENABLE, interrupts);
        break;

    case 2:
        WRITE_REG(UART2_CONFIG0, config_uart);
        WRITE_REG(UART2_CONFIG1, config_1_uart);
        WRITE_REG(UART2_CLK_DIVIDER, clk_divider);
        WRITE_REG(UART2_INTERRUPT_ENABLE, interrupts);
        break;

    default:
        // Handle invalid uart number
        printf("Invalid UART number, please choose either 0, 1, 2.");
        return;
  }

}