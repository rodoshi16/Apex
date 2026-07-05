#include <benchmark/benchmark.h>
#include "apex/order_book.hpp"
#include "apex/pool.hpp"

using namespace apex;

static Order make_limit(uint64_t id, Side side, int64_t price, uint64_t qty) {
    return Order{id, 0, price, qty, 0, side, OrderType::Limit, OrderStatus::New};
}

static Order make_market(uint64_t id, Side side, uint64_t qty) {
    return Order{id, 0, 0, qty, 0, side, OrderType::Market, OrderStatus::New};
}

// Benchmark 1: Add limit orders to the book
static void BM_AddLimitOrder(benchmark::State& state) {
    OrderBook book;
    uint64_t id = 0;
    std::vector<Event> events;
    events.reserve(16);

    for (auto _ : state) {
        events.clear();
        Order o = make_limit(id++, Side::Bid, 10000 - (id % 10), 100);
        book.add_order(o, events);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddLimitOrder)->Iterations(1000000);

// Benchmark 2: Cancel orders
static void BM_CancelOrder(benchmark::State& state) {
    OrderBook book;
    std::vector<Event> events;
    events.reserve(16);

    // Pre-fill the book
    for (uint64_t i = 0; i < 10000; ++i) {
        Order o = make_limit(i, Side::Bid, 10000 - (i % 20), 100);
        book.add_order(o, events);
        events.clear();
    }

    uint64_t id = 0;
    for (auto _ : state) {
        events.clear();
        book.cancel_order(id % 10000, events);
        // Re-add so we always have something to cancel
        Order o = make_limit(id % 10000, Side::Bid, 10000 - (id % 20), 100);
        book.add_order(o, events);
        id++;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelOrder)->Iterations(500000);

// Benchmark 3: Market order matching
static void BM_MarketOrderMatch(benchmark::State& state) {
    std::vector<Event> events;
    events.reserve(64);
    uint64_t id = 100000;

    for (auto _ : state) {
        state.PauseTiming();
        OrderBook book;
        events.clear();
        // Seed ask side
        for (int i = 0; i < 10; ++i) {
            int64_t price = 10000 - (id % 10);
            Order o = make_limit(id++, Side::Bid, price, 100);
            book.add_order(o, events);
            events.clear();
        }
        state.ResumeTiming();

        // Market buy — consumes liquidity
        Order market = make_market(id++, Side::Bid, 500);
        book.add_order(market, events);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MarketOrderMatch)->Iterations(100000);

// Benchmark 4: Object pool acquire/release vs raw new/delete
static void BM_PoolAcquire(benchmark::State& state) {
    static apex::ObjectPool<Order, 65536> pool;

    for (auto _ : state) {
        Order* o = pool.acquire();
        benchmark::DoNotOptimize(o);
        if (o) pool.release(o);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PoolAcquire)->Iterations(1000000);

static void BM_RawNewDelete(benchmark::State& state) {
    for (auto _ : state) {
        Order* o = new Order{};
        benchmark::DoNotOptimize(o);
        delete o;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RawNewDelete)->Iterations(1000000);

BENCHMARK_MAIN();