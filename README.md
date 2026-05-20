# Apex

> A low-latency limit order book and exchange simulation engine — C++20

Apex models the core infrastructure of a financial exchange. It implements price-time priority matching across a two-sided order book, synthetic market participants, and a nanosecond-resolution event pipeline — built to study the systems engineering problems that determine latency characteristics at scale.

## Architecture

<img width="1215" height="806" alt="178BB44D-9B0C-4733-BC2B-783A08F7603E_1_201_a" src="https://github.com/user-attachments/assets/252fc4fa-d4a7-45a6-9446-560b035ca9a4" />

## What's built

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

### Matching algorithm

When a buy order arrives: walk asks from lowest price upward. Match while `buyer_price >= ask_price`. Consume liquidity, emit fill events, remove empty levels. Remainder rests on book (limit) or is discarded (IOC).

Market orders bypass the price check entirely — they match at any available price.

## Order types

| Type | Behaviour |
|------|-----------|
| Limit | Match at price or better, rest remainder on book |
| Market | Match immediately at any price |
| IOC | Match available quantity, cancel remainder |
| FOK | Fill entire quantity or reject — coming soon |


## Verified behaviour

Build book: 2 bids, 2 asks
best bid: 9950 | best ask: 10000 | spread: 50 ✓
Market buy 100 → matches against best ask
1 fill event · ask depth -100 ✓
Cancel resting bid order
bid depth decreases · best bid updates ✓
Limit buy crosses spread → partial match + remainder rests
2 events · book state correct ✓


## Build

```bash
git clone https://github.com/rodoshi16/Apex.git
cd Apex
mkdir build && cd build
cmake .. -G Ninja
ninja
./apex
```

Requires: CMake 3.20+, C++20 compiler, Ninja

## References

- Avellaneda & Stoikov (2008) — *High-frequency trading in a limit order book*
- LMAX Disruptor (2011) — *Mechanical sympathy and lock-free concurrency*
- Cartea, Jaimungal & Penalva — *Algorithmic and High-Frequency Trading*
- Glosten & Milgrom (1985) — *Bid, ask and transaction prices in a specialist market*
