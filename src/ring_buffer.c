#include <stdint.h>
#include "ring_buffer.h"

void setup_ring_buffer(RingBuffer* ring_buffer, uint8_t* buffer, uint8_t buffer_size) {

  // define the ring buffer
  ring_buffer->buffer = buffer;
  ring_buffer->write_index = 0;
  ring_buffer->read_index = 0;
  // mask to insure indices are between 0 and the buffer size
  // if buffer is eight: 1000, then mask is 0111. 
  ring_buffer->mask  = buffer_size - 1;
}

// if the read index and the write index are at the same place
// then there's no data to read as there's no write
bool is_ring_buffer_empty(RingBuffer* ring_buffer) {
  return ring_buffer->read_index == ring_buffer->write_index;
}

bool read_ring_buffer(RingBuffer* ring_buffer, uint8_t* data) {
  
  if (is_ring_buffer_empty(ring_buffer))
    return false;

  // derefrence the data variable so we can write the value read_index points to
  *data = ring_buffer->buffer[ring_buffer->read_index];

  // increment the index and check if the buffer reached its full length
  ring_buffer->read_index = (ring_buffer->read_index + 1) % ring_buffer->mask;

  ring_buffer->is_full = false;

  return true;
}

bool write_ring_buffer(RingBuffer* ring_buffer, uint8_t data) {

  // if the buffer is full return
  if (ring_buffer->is_full)
    return false;

  // get the buffer memory location that the write index writes at so we
  // can write the data parameter to it
  ring_buffer->buffer[ring_buffer->write_index] = data;

  // increment the index of the write index and check if the buffer has reached its full length
  ring_buffer->write_index = (ring_buffer->write_index + 1) % ring_buffer->mask;

  if (ring_buffer->write_index == ring_buffer->read_index)
    ring_buffer->is_full = true;

  return true;
}