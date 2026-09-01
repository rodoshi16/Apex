***The Big Picture First***

Imagine a stock exchange like Nasdaq. When someone wants to buy or sell a stock, they send an order to Nasdaq. Nasdaq's job is to find a match — someone selling at the price a buyer wants to pay — and execute the trade.

Apex is a simulation of that entire system. Not a real exchange with real money, but a technically accurate model of how one works internally. Every file in your repo is one piece of that system.


**include/apex/order.hpp — What an order looks like**

What it is: A blueprint for a single order. Like a form someone fills out when they want to buy or sell.

Real world analogy: Imagine you call your broker and say "I want to buy 100 shares of Apple at $150." That entire request — who you are, what you want, at what price, how many — is one Order.

What's inside:

order_id       — a unique ticket number for this order
timestamp_ns   — exactly when it arrived, in nanoseconds
price          — what price they want, stored as a whole number
                 $100.00 is stored as 10,000,000
                 (we never use decimals — explained below)
quantity       — how many shares
filled_quantity — how many have already been traded
side           — are they buying (Bid) or selling (Ask)?
type           — what kind of order is this?
status         — is it new, partially filled, fully filled, cancelled?

Why no decimals for price?
Computers are bad at decimal math. 0.1 + 0.2 in a computer doesn't always equal exactly 0.3 — it's something like 0.30000000000000004. In a trading system that means money appearing or disappearing from nowhere. So we store prices as whole numbers — basis points — where $1.00 = 100 and $100.00 = 10,000,000. All the math is exact.

The four order types:

Limit — "Buy at $100 or cheaper, otherwise wait." The most common type. If no one is selling at $100, the order sits and waits.
Market — "Buy immediately at whatever price is available." No waiting, guaranteed to fill.
IOC (Immediate or Cancel) — "Fill whatever you can right now, cancel the rest." Like a market order but you don't want leftovers sitting around.
FOK (Fill or Kill) — "Fill my entire order right now or don't fill any of it." All or nothing. No partial fills allowed.
include/apex/event.hpp — The audit trail

What it is: Every time something happens inside the engine, it writes down what happened. An Event is one of those records.

Real world analogy: Imagine a security camera in a bank vault. Every time a door opens, a transaction happens, anything changes — it gets recorded with a timestamp. Events are that recording for your engine.

What gets recorded:

OrderAdded — a new order joined the book
OrderCancelled — an order was removed before being filled
OrderPartiallyFilled — an order was half-filled
OrderFilled — an order was completely filled
TradeExecuted — a trade happened between a buyer and seller

Every event has a nanosecond timestamp — billionths of a second precision. This matters because at 5 million orders per second, things happen incredibly fast and you need to know the exact sequence.

Why this matters for your project: Your analytics layer — the Python dashboard you'll build later — reads these events to answer questions like "how long does a typical order wait before being filled?" and "how does the spread change when more informed traders arrive?"

**include/apex/price_level.hpp + src/price_level.cpp — One row in the book**

What it is: The order book is organized by price. A PriceLevel represents all the orders sitting at one specific price point.

Real world analogy: Imagine an auction. Everyone willing to pay exactly $100 stands in one line. Everyone willing to pay $99 stands in another line. A PriceLevel is one of those lines.

What a price level looks like:

Price $100.00:
[Person A wants 200 shares] → [Person B wants 150 shares] → [Person C wants 300 shares]

This is a queue — first in, first out. Person A arrived first so they get filled first. This is called price-time priority — at the same price, whoever arrived earlier wins.

The clever part — why O(1) cancel matters:

When someone cancels an order, you need to find it and remove it. The naive way is to scan through the entire line looking for them — if there are 10,000 people in line, that's 10,000 checks. Called O(n) — gets slower as the line gets longer.

Your implementation uses two data structures together:

The line (std::list):
[Order A] ↔ [Order B] ↔ [Order C]

The lookup table (std::unordered_map):
Order ID 1 → points directly to Order A in the line
Order ID 2 → points directly to Order B in the line
Order ID 3 → points directly to Order C in the line

When someone cancels Order B: look up Order B in the table instantly, jump directly to it in the line, remove it. No scanning. Always one step regardless of how many orders are in the line. Called O(1).

At hundreds of thousands of cancels per second, this difference is the difference between a working system and a broken one.

Three operations:

add_order — join the back of the line, add your pointer to the lookup table
cancel_order — look yourself up, step out of the line
fill — consume from the front of the line (first person gets filled first)
include/apex/order_book.hpp + src/order_book.cpp — The full exchange

What it is: The complete order book — both sides of the market, the matching algorithm, everything.

Real world analogy: The order book is the whiteboard at an auction showing all current buyers and sellers:

SELLERS (Ask side — lowest price first):
$100.03  →  200 shares available
$100.02  →  500 shares available
$100.01  →  100 shares available
─────────────── spread ───────────────
$100.00  →  300 shares wanted
$99.99   →  400 shares wanted
$99.98   →  250 shares wanted
BUYERS (Bid side — highest price first)

The gap between the lowest ask ($100.01) and highest bid ($100.00) is called the spread. In your engine right now it's 50 basis points — $0.005.

Why bids are sorted highest first, asks lowest first:

When a buyer arrives, you want to give them the best deal — the cheapest available price. So you look at asks from lowest upward. When a seller arrives, you want to give them the best deal — the highest available price. So you look at bids from highest downward.

std::greater on the bid side reverses the default sorting so bids_.begin() always gives you the highest bid instantly.

The matching algorithm — step by step:

A market buy order for 150 shares arrives:

Look at the lowest ask — $100.01, 100 shares available
Buyer will pay any price, so prices cross — trade happens
Fill all 100 shares at $100.01, emit a fill event
That price level is now empty — remove it
Still need 50 more shares — look at next ask — $100.02
Fill 50 shares from the 500 available at $100.02
Order is complete — emit filled event
$100.02 level still has 450 shares remaining

A limit buy order for 100 shares at $100.00 arrives:

Look at lowest ask — $100.01
Buyer only wants to pay $100.00, but cheapest seller wants $100.01 — prices don't cross
No match possible — order rests on the bid side at $100.00
Emit OrderAdded event

FOK special case:
Before touching anything, check if the full quantity is available. If not, emit a cancel event and return. The book is never modified. This is the pre-flight liquidity check.

The order_locations_ map:
A hash map from order ID to which side and price the order is resting at. When a cancel comes in for order 12345, you instantly know "it's on the bid side at price 9950" — O(1) routing to the right price level.

**include/apex/pool.hpp — The memory pool**

What it is: A pre-allocated collection of Order objects sitting ready to use, so you never have to ask the operating system for memory during trading.

Real world analogy: Imagine a restaurant. The naive approach: every time a customer sits down, go to the kitchen and build a new table from scratch. Obviously slow. The smart approach: have 50 tables already built and ready. When a customer arrives, hand them a table. When they leave, take the table back and clean it for the next customer.

Your pool is 65,536 pre-built Order slots. Acquiring one takes 8.96 nanoseconds. Building one fresh from the heap takes 24.1 nanoseconds. 2.7x faster.

Why heap allocation is slow:
When you call new Order() in C++, the operating system has to find free memory, mark it as used, update internal bookkeeping, and potentially lock a mutex so other threads don't get the same memory. All of that takes time. At 5 million orders per second, that overhead is catastrophic.

The lock-free free list:
Orders waiting to be used are connected in a chain. To acquire one, you atomically grab the head of the chain and update the pointer — no locks, no operating system involvement. To release one, you push it back onto the front of the chain.

**include/apex/disruptor.hpp — The lock-free pipeline**

What it is: A ring buffer — a circular queue — that passes orders between threads without any locks.

Real world analogy: Imagine a sushi conveyor belt. The chef (producer) puts plates on the belt continuously. The customer (consumer) takes plates off in order. They never need to talk to each other or wait for each other — the belt handles the coordination. No one has to stop and ask "is it okay if I put a plate on?" They just put it on and the customer picks it up when they get to it.

Why not just use a mutex:
A mutex is like having one door into a room that only one person can go through at a time. At 5 million orders per second, threads are constantly waiting at that door. Each wait is 50-100 nanoseconds minimum. That's your entire latency budget gone.

How the disruptor works:

Ring buffer — 1024 slots in a circle:
[slot 0][slot 1][slot 2]...[slot 1023][slot 0][slot 1]...
                                        ↑ wraps around

Two sequence numbers — like scoreboard counters:

Producer sequence: "I have filled up to slot number X"
Consumer sequence: "I have processed up to slot number Y"

Producer wants to add an order: increment their sequence, write into that slot, done. Consumer wants to process: check if producer is ahead, if yes read the next slot, increment their sequence, done. No locks anywhere.

The false sharing fix — your interview talking point:

cpp
alignas(64) std::atomic<int64_t> producer_sequence_;
alignas(64) std::atomic<int64_t> consumer_sequence_;

A CPU cache line is 64 bytes. If two variables share a cache line, every time one thread writes to its variable, it invalidates the other thread's cache — even though they're touching different variables. They're fighting over the same 64-byte chunk of memory.

alignas(64) forces each sequence number onto its own cache line. The producer and consumer now operate on completely independent memory. Before this fix: threads constantly invalidating each other's cache. After: zero interference.

The power-of-two trick:
Buffer size is 1024 — a power of two. To find which slot a sequence number maps to, instead of dividing (slow), you use a bitmask: index = sequence & 1023. This is the same as sequence % 1024 but takes one CPU instruction instead of many.

What The Engine Does End to End

Put it all together:

An order arrives at the Gateway thread
It's written into the Disruptor ring buffer — lock-free, nanoseconds
The Matching thread reads it from the buffer
It's sent to the OrderBook
The OrderBook finds the right PriceLevel on the opposite side
The PriceLevel fills orders FIFO, emitting Events
Unfilled remainder rests on the book or is cancelled depending on order type
Events flow to the analytics layer (Python — coming soon)
The Five Questions You Must Be Able To Answer

1. Why alignas(64) on the disruptor sequences?
Cache lines are 64 bytes. Without it, producer and consumer sequences share a line and constantly invalidate each other's cache — false sharing. alignas(64) puts each on its own line.

2. Why std::list + iterator map for cancel?
List gives stable iterators — pointers that stay valid when other elements are removed. The map gives O(1) lookup from order ID to that pointer. Together: find and remove any order in O(1) regardless of how many orders exist.

3. Why std::greater on the bid side?
std::map sorts ascending by default. Bids need to be sorted descending — highest price first — so the best bid is always at begin(). std::greater reverses the sort.

4. Why integer prices not floating point?
Floating point math is imprecise — 0.1 + 0.2 ≠ 0.3 exactly. In a trading system that means money appearing or disappearing. Integer basis points are exact.

5. What does FOK check before touching the book?
Whether the total available quantity on the opposite side at acceptable prices is >= the requested quantity. If not, emit a cancel event and return immediately — the book is never modified.