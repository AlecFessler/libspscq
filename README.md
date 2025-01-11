# Single-Producer Single-Consumer Lock-Free Queue

A highly optimized single-producer single-consumer (SPSC) lock-free (and wait-free) queue implementation in C and C++, focusing on maximizing throughput by minimizing cache coherency traffic and maximizing cache locality.

## Performance

Performance measurements on a Ryzen 9 7950x3d with producer and consumer threads pinned to different cores on CCD0:

| Implementation | L1 Misses | IPC | Time per Transfer | Transfers/Second |
|----------------|-----------|-----|-------------------|-----------------|
| Baseline Unoptimized | 1.98% | 0.76 | 6.19 ns | 163.22M |
| Cache Aligned Head/Tail | 2.13% | 0.66 | 6.2 ns | 161.49M |
| Cache Aligned + Local Cached Head/Tail | 1.84% | 1.37 | 3.22 ns | 310.17M |
| Locality Optimized + Local Cached Head/Tail | 1.44% | 1.6 | 3.04 ns | 328.73M |

These metrics were chosen as they show the most significant changes between implementations. Other performance indicators remained relatively consistent across versions.

## Design & Optimizations

### Evolution of the Implementation

The implementation journey starts with a basic unoptimized queue structure:

```c
struct spsc_q {
    uint32_t cap;
    void* buf;
    _Atomic uint32_t head;
    _Atomic uint32_t tail;
};
```

Popular implementations like Folly and Boost improve on this by adding cache line alignment to prevent false sharing between the head and tail. However, this approach creates a trade-off: while it prevents false sharing, it requires each queue operation to touch 3 cache lines (since each field with cache line alignment must start on a new cache line), partially offsetting the performance gained.

Erik Rigtorp's implementation introduced a significant throughput improvement through cached versions of head and tail indices. While this increases potential cache line touches to 3-4 per operation, it dramatically reduces cache coherency traffic by allowing threads to operate using their cached view of the other thread's progress. The variable number of cache line touches occurs because threads only need to check the other thread's actual position when the queue appears full or empty.

My implementation builds on Rigtorp's cached index approach while further optimizing for locality. By carefully arranging the data structures, each thread touches only 1-2 cache lines per operation while maintaining the benefits of both cache alignment and head/tail caching. This resulted in small but consistently measurable improvements across all measured metrics compared to Rigtorp's implementation, primarily due to maximizing locality as much as possible within the algorithm's requirements.

### Cache-Conscious Thread Separation

This implementation splits the queue into separate producer and consumer structures, each carefully designed to fit within a single 64-byte cache line. This approach ensures that each thread can access all frequently needed data (index, cached opposite index, capacity, and buffer pointer) without requiring multiple cache line loads.

An identical performance outcome can be achieved with a single struct approach like so:

```c
struct spsc_q {
  _Atomic uint32_t head;
  uint32_t cached_tail;
  uint32_t producer_cap;
  void* producer_buf;
  alignas(CACHE_LINE_SIZE) char padding[CACHE_LINE_SIZE];

  _Atomic uint32_t tail;
  uint32_t cached_head;
  uint32_t consumer_cap;
  void* consumer_buf;
  alignas(CACHE_LINE_SIZE) char padding[CACHE_LINE_SIZE];
};
```

The key points are that each thread's 'workspace' falls within its own single cache line, separate from the other threads. The duplicated capacity and buffer go into what would otherwise be padding, so have no real cost.

### Head/Tail Caching Optimization

Building on Erik Rigtorp's approach (https://github.com/rigtorp/SPSCQueue), each thread maintains a local cache of the other thread's position. This cache is only updated when the queue appears to be full (for the producer) or empty (for the consumer).

This optimization dramatically reduces cross-core communication, as threads can often operate independently using their cached view of the other thread's progress. Cache coherency traffic only occurs when threads are actually about to interfere with each other.

### Cross-Language Compatibility

The implementation includes both C and C++ versions that compile to compatible machine code, allowing usage across the language barrier in shared memory scenarios. While the compiled operations remain functionally identical, there are minor differences:
- The C++ compiler uses xchg instead of mov instructions
- C++ std::atomic adds runtime alignment checks
- Struct layout and memory ordering semantics remain identical

### Implementation Note

An interesting aspect of this implementation on x86 platforms is that due to the processor's memory ordering model (which prevents reordering of store-store, load-load, and load-store operations, but allows store-load reordering), and the fact that naturally aligned words already have atomic loads and stores, the compiled assembly essentially results in a standard ring buffer implementation. While it would be technically possible to achieve similar results using std::atomic_signal_fence() to prevent compiler reordering of specific instructions (like reading the tail followed by storing the incremented tail, or writing to the head followed by storing the incremented head), this approach is not recommended. The key insight here is that while x86 naturally provides the necessary memory ordering guarantees at the processor level for this specific case, we still need explicit mechanisms to prevent compiler reordering. Using atomics makes the code's intent explicit and maintains portability across architectures. This compiler explorer link shows the compiled assembly for the regular version with atomics, and the version doing what I mentioned above:

https://godbolt.org/z/bcd9jcssf

The compiled assembly is not quite identical, but what's important to note is that layout of the structs are still identical, and that the atomics and memory orderings have not output any particular instructions, as in x86, only acquire-release and sequential consistency orderings will result in an MFENCE, and as mentioned above, loads and stores here are naturally atomic. Again, I don't recommend doing this, it's not an optimization, and it's not portable, it's just an interesting aside that gives insight into why this algorithm is such an efficient lock-free structure; The required synchronization is so lightweight, it effectively compiles into a standard ring buffer.

## Usage

```c
// Initialize the queue
struct producer_q producer;
struct consumer_q consumer;
void* buffer = malloc(size * sizeof(void**));
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
