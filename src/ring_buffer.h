#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_RING_BUFFER_SIZE 32

typedef struct {
  uint8_t* buffer;    
  uint32_t mask;         
  uint32_t read_index;   
  uint32_t write_index;
  bool is_full;   
} RingBuffer;

// Functions that operate on RingBuffer
#ifdef __cplusplus
extern "C" {
#endif

void setup_ring_buffer(RingBuffer* ring_buffer, uint8_t* buffer, uint8_t buffer_size);
bool is_ring_buffer_empty(RingBuffer* ring_buffer);
bool read_ring_buffer(RingBuffer* ring_buffer, uint8_t* data);
bool write_ring_buffer(RingBuffer* ring_buffer, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif