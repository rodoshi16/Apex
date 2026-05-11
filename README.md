# Apex

A low-latency limit order book and exchange simulation engine written in C++20.

Implements price-time priority matching with support for limit, market, and IOC 
order types. Built to study the systems engineering problems inside financial 
exchanges — specifically, how data structure choices and concurrency design 
determine latency characteristics at scale.

---

## Architecture
Bid Side (highest price first)        Ask Side (lowest price first)
─────────────────────────────         ─────────────────────────────
$100.02  [Order A] → [Order B]        $100.03  [Order C]
$100.01  [Order D]                    $100.05  [Order E] → [Order F]
↕ matching engine ↕

Incoming orders are matched against the opposite side using price-time priority.
Every state transition emits a timestamped Event consumed by the analytics layer.

---

## Data Structures

### PriceLevel
Each price point maintains a FIFO queue of resting orders implemented as a 
doubly-linked list (`std::list`) with a parallel hash map from order ID to 
list iterator. This gives O(1) insertion, O(1) cancellation, and O(1) fill 
from the front — critical at exchange-level throughput.

Naive implementations use linear scan for cancellation (O(n)). At hundreds of 
thousands of cancels per second, this is catastrophically slow.

### OrderBook
Two `std::map` containers — bids sorted descending (`std::greater`), asks 
sorted ascending. Best bid is always `bids_.begin()`, best ask is always 
`asks_.begin()` — O(log n) insertion, O(1) best price lookup.

A secondary hash map (`order_locations_`) maps order ID to side and price 
level for O(1) cancel routing.

---

## Order Types

| Type   | Behavior |
|--------|----------|
| Limit  | Match at specified price or better, rest remainder on book |
| Market | Match immediately at any available price |
| IOC    | Match what is available immediately, cancel remainder |
| FOK    | Fill entire quantity or reject completely (coming soon) |

---

## Event Log

Every state transition emits a typed, nanosecond-timestamped Event:

- `OrderAdded` — resting limit order entered the book
- `OrderCancelled` — order removed before fill
- `OrderPartiallyFilled` — order matched against part of a price level
- `OrderFilled` — order fully matched
- `TradeExecuted` — a trade occurred between two counterparties

The event log is the interface between the matching engine and the analytics 
layer. The engine is a black box that accepts orders and emits events.

---

## Verified Behavior
TEST 1: Build book with 2 bids and 2 asks
best bid: 9950 | best ask: 10000 | spread: 50 ✓
TEST 2: Market buy of 100 matches against best ask
1 fill event emitted, ask depth reduced by 100 ✓
TEST 3: Cancel resting bid order
bid depth reduced, best bid updates correctly ✓
TEST 4: Limit buy crosses spread, partially matches, remainder rests
2 events, book state correct ✓

---

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

---

## Roadmap

- [x] Order and Event data models
- [x] PriceLevel with O(1) cancel
- [x] OrderBook with price-time priority matching
- [x] Limit, market, IOC order types
- [ ] Lock-free disruptor ring buffer for inter-thread communication
- [ ] CPU thread pinning and cache line alignment
- [ ] Avellaneda-Stoikov market maker simulation
- [ ] Informed trader and noise trader agents
- [ ] Ornstein-Uhlenbeck mid-price process
- [ ] Analytics layer — spread, market impact, queue position fill rates
- [ ] Latency benchmarks — p50, p99, p99.9 under load

---

## References

- Avellaneda & Stoikov (2008) — *High-frequency trading in a limit order book*
- LMAX Disruptor (2011) — *Mechanical sympathy in concurrent systems*
- Cartea, Jaimungal & Penalva — *Algorithmic and High-Frequency Trading*
