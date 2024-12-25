# Single-Producer Single-Consumer Lock-Free Queue

A highly optimized single-producer single-consumer (SPSC) lock-free queue implementation in C, focusing on maximizing throughput by minimizing cache coherency traffic and carefully managing CPU cache utilization.

## Performance

On a Ryzen 9 7950x3d with producer and consumer threads pinned to different cores on CCD0:
- 300 million transfers (600 million total operations) per second
- 3.35 nanoseconds per queue/dequeue operation
- 1.6 instructions per cycle
- 1.49% L1 cache misses
- 0.77% branch misses

## Design & Optimizations

### Cache-Conscious Thread Separation

This implementation splits the queue into separate producer and consumer structures, each carefully designed to fit within a single 64-byte cache line. This approach ensures that each thread can access all frequently needed data (index, cached opposite index, capacity, and buffer pointer) without requiring multiple cache line loads.

The key insight is that while traditional implementations often focus on preventing false sharing through cache line alignment of individual members, this can actually harm performance by forcing each operation to touch multiple cache lines. By keeping all thread-local data in a single cache line, we minimize cache coherency traffic while still preventing false sharing between threads.

### Head/Tail Caching Optimization

This implementation uses the head/tail caching optimization, an approach I discovered in Erik Rigtorp's implementation (https://github.com/rigtorp/SPSCQueue). Instead of checking the other thread's progress on every operation, each thread maintains a local cache of the other thread's position. This cache is only updated when the queue appears to be full (for the producer) or empty (for the consumer).

This optimization dramatically reduces cross-core communication, as threads can often operate independently using their cached view of the other thread's progress. Cache coherency traffic only occurs when threads are actually about to interfere with each other.

### Careful Memory Ordering

The implementation uses precisely chosen memory ordering semantics to ensure correctness with minimal synchronization overhead:
- Relaxed ordering when loading a thread's own index (as each thread is the sole writer of its index)
- Acquire ordering when loading the other thread's index (to see updates from the other thread)
- Release ordering for the producer when storing its index (ensuring the consumer sees the correct data)
- Relaxed ordering for the consumer when storing its index (since the producer doesn't need any particular ordering in the dequeue operation for correctness)

### Cache Line Alignment Behavior

Through extensive benchmarking, I discovered a significant performance impact related to cache line alignment on AMD processors. Placing the cache line alignment directive (alignas(CACHE_LINE_SIZE)) on the final field of the struct ensures that field begins at a cache line boundary, with the struct's data fields packed tightly in the preceding space. When this field is the padding (or alternatively, the buffer pointer), this arrangement empirically provides optimal performance. Moving the alignment directive elsewhere or removing it entirely can reduce throughput by up to 50% on AMD processors, suggesting this layout interacts favorably with specific microarchitectural behaviors. While the exact mechanism isn't fully understood, the performance benefit is consistently reproducible in benchmarks.

### Memory Layout Trade-offs

The implementation duplicates the buffer pointer and capacity in both the producer and consumer structs to ensure each thread has all necessary data in a single cache line. Since each thread needs its own cache line anyway, this approach efficiently uses space that would otherwise be padding while eliminating the need for additional cache line loads during normal operation.

## Usage

```c
// Initialize the queue
struct producer_q producer;
struct consumer_q consumer;
void* buffer = aligned_alloc(CACHE_LINE_SIZE, size * sizeof(void*));
spsc_queue_init(&producer, &consumer, buffer, size);

// Producer thread
spsc_enqueue(&producer, data);

// Consumer thread
void* data = spsc_dequeue(&consumer);
```

Note that the actual capacity of the queue is size - 1, as one slot must remain empty to distinguish between full and empty states.

## Requirements

- C11 or later (for `_Atomic` and `alignas`)
- 64-byte cache lines (adjustable via `CACHE_LINE_SIZE`)

## License

MIT License
