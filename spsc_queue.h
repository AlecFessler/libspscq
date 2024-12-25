// © 2024 Alec Fessler
// MIT License
// See LICENSE file in the project root for full license information.

#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include <stdatomic.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define CACHE_LINE_SIZE 64

struct producer_q {
  _Atomic size_t head;
  _Atomic size_t* tail_ptr;
  size_t cached_tail;
  size_t cap;
  void* buf;
  alignas(CACHE_LINE_SIZE) char padding[
    CACHE_LINE_SIZE
    - (sizeof(_Atomic size_t)
    + sizeof(_Atomic size_t*)
    + sizeof(size_t) * 2
    + sizeof(void*))
  ];
};

struct consumer_q {
  _Atomic size_t tail;
  _Atomic size_t* head_ptr;
  size_t cached_head;
  size_t cap;
  void* buf;
  alignas(CACHE_LINE_SIZE) char padding[
    CACHE_LINE_SIZE
    - (sizeof(_Atomic size_t)
    + sizeof(_Atomic size_t*)
    + sizeof(size_t) * 2
    + sizeof(void*))
  ];
};

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

#endif // SPSC_QUEUE_H
