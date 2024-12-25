#include <errno.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "spsc_queue.h"

int spsc_queue_init(
  struct producer_q* pq,
  struct consumer_q* cq,
  void* buf,
  size_t size
) {
  atomic_store_explicit(&pq->head, 0, memory_order_relaxed);
  pq->tail_ptr = &cq->tail;
  pq->cached_tail = 0;
  pq->cap = size;
  pq->buf = buf;

  atomic_store_explicit(&cq->tail, 0, memory_order_relaxed);
  cq->head_ptr = &pq->head;
  cq->cached_head = 0;
  cq->cap = size;
  cq->buf = buf;

  return 0;
}

int spsc_enqueue(
  struct producer_q* q,
  void* data
) {
  // load head with relaxed ordering, we're the only writer
  size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);

  size_t next = head + 1;
  if (next == q->cap)
    next = 0;

  if (next == q->cached_tail) { // check if the queue appears full
    q->cached_tail = atomic_load_explicit(q->tail_ptr, memory_order_acquire);
    if (next == q->cached_tail) // check if the queue actually is full
      return -EAGAIN;
  }

  void** slot = (void**)q->buf + head;
  *slot = data;

  // update the head with release semantics so the consumer
  // sees the assignment of the ptr to the slot before the
  // atomic store of the head index
  atomic_store_explicit(&q->head, next, memory_order_release);

  return 0;
}

void* spsc_dequeue(struct consumer_q* q) {
  // load tail with relaxed ordering, we're the only writer
  size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);

  if (tail == q->cached_head) { // check if the queue appears empty
    q->cached_head = atomic_load_explicit(q->head_ptr, memory_order_acquire);
    if (tail == q->cached_head) // check if the queue actually is empty
      return NULL;
  }

  void** slot = (void**)q->buf + tail;
  void* data = *slot;

  size_t next_tail = tail + 1;
  if (next_tail == q->cap)
    next_tail = 0;

  // unlike in the enqueue, where we need the ptr assignment to
  // 'happen before' the atomic increment, there's no operation in
  // this function that needs 'happens before' ordering wrt the atomic store
  // so relaxed ordering is sufficient, the consumer thread will see the
  // correct data ptr since it was written to it's own cache
  atomic_store_explicit(&q->tail, next_tail, memory_order_relaxed);

  return data;
}

size_t sizeof_producer_q() {
  return sizeof(struct producer_q);
}

size_t alignof_producer_q() {
  return alignof(struct producer_q);
}

size_t sizeof_consumer_q() {
  return sizeof(struct consumer_q);
}

size_t alignof_consumer_q() {
  return alignof(struct consumer_q);
}
