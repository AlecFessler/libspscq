// © 2024 Alec Fessler
// MIT License
// See LICENSE file in the project root for full license information.

#ifndef SPSC_QUEUE_HPP
#define SPSC_QUEUE_HPP

#include <cstddef>

extern "C" {

struct producer_q;
struct consumer_q;

int spsc_queue_init(
  struct producer_q* pq,
  struct consumer_q* cq,
  void* buf,
  size_t size
);
int spsc_enqueue(
  struct producer_q* q,
  void* data
);
void* spsc_dequeue(struct consumer_q* q);

size_t sizeof_producer_q();
size_t alignof_producer_q();
size_t sizeof_consumer_q();
size_t alignof_consumer_q();
}

#endif // SPSC_QUEUE_HPP
