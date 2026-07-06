# Apex
> A low-latency exchange simulation engine — C++20

Apex models the core infrastructure of a financial exchange. It implements price-time priority matching across a two-sided order book, a lock-free disruptor ring buffer for inter-thread order passing, and a nanosecond-resolution event pipeline — built to study the systems engineering problems that determine latency characteristics at scale.

---

## Benchmark results

| Operation | Latency | Throughput |
|---|---|---|
| Add limit order | 199ns | 5.03M / sec |
| Cancel order | 157ns | 6.37M / sec |
| Market order match | 1922ns | 520K / sec |
| Object pool acquire | 8.96ns | 111M / sec |
| Raw new/delete | 24.1ns | 41.7M / sec |

**Pool vs heap: 2.7x faster.** Measured via Google Benchmark at 1M iterations with `-O2 -DNDEBUG`.

---

## Architecture

<img width="1215" height="806" alt="178BB44D-9B0C-4733-BC2B-783A08F7603E_1_201_a" src="https://github.com/user-attachments/assets/252fc4fa-d4a7-45a6-9446-560b035ca9a4" />

---

## What's built

### Disruptor ring buffer — `include/apex/disruptor.hpp`

Lock-free inter-thread communication using a 1024-slot circular buffer. Producers and consumers coordinate via atomic sequence numbers — no mutexes, no heap allocation at runtime.

The critical detail: producer and consumer sequences are each forced onto their own 64-byte cache line with `alignas(64)`. Without this, every producer write invalidates the consumer's cache line — false sharing — causing performance collapse under load. With it, the two cores operate independently.

```cpp
alignas(64) std::atomic<int64_t> producer_sequence_;
alignas(64) std::atomic<int64_t> consumer_sequence_;
```

Ring buffer index computed via bitmask — zero division: `index = sequence & (SIZE - 1)`.

### Order book — `include/apex/order_book.hpp`

Two `std::map` containers — bids sorted descending (`std::greater`), asks sorted ascending. Best bid is always `bids_.begin()`, best ask always `asks_.begin()` — O(log n) insert, O(1) best-price lookup.

A secondary hash map (`order_locations_`) maps order ID → side + price level for O(1) cancel routing without scanning.

### Price level — `include/apex/price_level.hpp`

Each price point holds a FIFO queue of resting orders:

Price $100.00:
[Order 1: 200] → [Order 2: 150] → [Order 3: 300]
↑                 ↑                 ↑
order_map[1]      order_map[2]      order_map[3]

`std::list` gives stable iterators. The parallel `std::unordered_map` stores each order's iterator, making cancel O(1). Naive implementations scan linearly — catastrophic at exchange throughput.

### Object pool — `include/apex/pool.hpp`

Pre-allocated lock-free free list eliminates heap interaction on the hot path. Acquire and release via compare-and-swap. Result: 8.96ns vs 24.1ns per allocation — 2.7x faster than `new`/`delete`.

### Matching algorithm

When a buy order arrives: walk asks from lowest price upward. Match while `buyer_price >= ask_price`. Consume liquidity, emit fill events, remove empty levels. Remainder rests on book (limit) or discarded (IOC/FOK).

FOK performs a pre-flight liquidity check before touching the book — if full fill is impossible, the order is rejected entirely with zero book modification.

---

## Order types

| Type | Behaviour |
|------|-----------|
| Limit | Match at price or better, rest remainder on book |
| Market | Match immediately at any price |
| IOC | Match available quantity, cancel remainder |
| FOK | Pre-flight liquidity check — fill entirely or reject, book untouched |

---

## Verified behaviour

Order book construction
best bid: 9950 | best ask: 10000 | spread: 50 ✓
Market order matches against best ask
1 fill event · ask depth -100 ✓
Cancel resting bid
bid depth decreases · best bid updates ✓
Limit order crosses spread → partial match + remainder rests
2 events · book state correct ✓
FOK rejected — insufficient liquidity
1 cancel event · book untouched · ask depth unchanged ✓
FOK accepted — sufficient liquidity across 2 price levels
2 fill events · book fully consumed ✓
Disruptor: 1000 orders published → 1000 processed
producer/consumer sequence tracking correct ✓
Disruptor: buffer fills exactly 1024 slots then rejects ✓

---

## Design decisions

**Why `std::list` + iterator map for price levels?**
Cancel must be O(1). A vector requires linear scan to find the order — at hundreds of thousands of cancels per second this is catastrophic. The linked list with a parallel iterator map gives O(1) lookup and O(1) removal. The tradeoff is pointer indirection per node — acceptable since cancel is not the absolute hottest path.

**Why integer prices?**
Floating point arithmetic is not associative. Two different orderings of the same operations can produce different results. In a matching engine, that means money appearing or disappearing. Prices are stored as `int64_t` in basis points (1/100th of a cent) — $100.00 = 10,000,000. All arithmetic is exact.

**Why single-threaded matching?**
The order book is stateful and not safely shareable. Two threads matching simultaneously would require locking the entire book — destroying the performance advantage. The correct design is a single matching thread consuming from a lock-free queue. Concurrency lives in the producer layer, not inside the book.

**Why `alignas(64)` on disruptor sequences?**
A cache line is 64 bytes. If producer and consumer sequences share a cache line, every write by the producer invalidates the consumer's cache — and vice versa — even though they're touching different variables. This is false sharing. `alignas(64)` forces each onto its own line, eliminating the cross-core invalidation traffic.

---

## Build

```bash
git clone https://github.com/rodoshi16/Apex.git
cd Apex
mkdir build && cd build
cmake .. -G Ninja
ninja
./apex                    # correctness tests
./benchmarks/apex_bench   # latency benchmarks
```

Requires: CMake 3.20+, C++20 compiler, Ninja

---

## Roadmap

- [x] Order and Event data models
- [x] PriceLevel — O(1) insert, cancel, fill
- [x] OrderBook — price-time priority matching
- [x] Limit, market, IOC, FOK order types
- [x] Lock-free LMAX Disruptor ring buffer
- [x] Lock-free object pool — 2.7x faster than heap
- [x] Google Benchmark integration — real latency numbers
- [ ] CPU thread pinning · cache-line alignment · huge pages
- [ ] Circular price level array — O(1) vs std::map benchmark
- [ ] Ornstein-Uhlenbeck mid-price process
- [ ] Avellaneda-Stoikov market maker agent
- [ ] Poisson informed traders · noise traders
- [ ] Analytics — spread dynamics, market impact, queue fill rates
- [ ] Latency histograms — p50 / p99 / p99.9 under concurrent load
- [ ] Streamlit live dashboard

---

## References

- Avellaneda & Stoikov (2008) — *High-frequency trading in a limit order book*
- LMAX Disruptor (2011) — *Mechanical sympathy and lock-free concurrency*
- Cartea, Jaimungal & Penalva — *Algorithmic and High-Frequency Trading*
- Glosten & Milgrom (1985) — *Bid, ask and transaction prices in a specialist market*
