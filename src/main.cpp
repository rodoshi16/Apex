#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "apex/order.hpp"
#include "apex/event.hpp"
#include "apex/order_book.hpp"
#include "apex/disruptor.hpp"

using namespace apex;

int main() {
    Disruptor disruptor;
    OrderBook book;
    std::vector<Event> events;
    events.reserve(64);

    std::atomic<bool> done{false};
    std::atomic<int64_t> orders_processed{0};

    // Consumer thread — the matching engine
    std::thread consumer([&]() {
        Order order;
        while (!done.load(std::memory_order_relaxed) ||
               disruptor.producer_sequence() > disruptor.consumer_sequence()) {
            if (disruptor.consume(order)) {
                events.clear();
                book.add_order(order, events);
                orders_processed.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Producer — submits orders into the ring buffer
    std::cout << "=== TEST 1: Single producer publishing orders ===\n";
    uint64_t id = 0;
    int64_t  published = 0;

    // Publish 500 bids and 500 asks
    for (int i = 0; i < 500; ++i) {
        Order bid{id++, 0, int64_t(9900 + (i % 10)), 100, 0,
                  Side::Bid, OrderType::Limit, OrderStatus::New};
        while (!disruptor.publish(bid)) {} // spin if full
        published++;
    }

    for (int i = 0; i < 500; ++i) {
        Order ask{id++, 0, int64_t(10000 + (i % 10)), 100, 0,
                  Side::Ask, OrderType::Limit, OrderStatus::New};
        while (!disruptor.publish(ask)) {}
        published++;
    }

    // Wait for consumer to drain
    while (orders_processed.load() < published) {
        std::this_thread::yield();
    }

    done.store(true, std::memory_order_relaxed);
    consumer.join();

    std::cout << "  published:  " << published << "\n";
    std::cout << "  processed:  " << orders_processed.load() << "\n";
    std::cout << "  best bid:   " << book.best_bid().value_or(-1) << "\n";
    std::cout << "  best ask:   " << book.best_ask().value_or(-1) << "\n";
    std::cout << "  bid depth:  " << book.bid_depth() << "\n";
    std::cout << "  ask depth:  " << book.ask_depth() << "\n";

    std::cout << "\n=== TEST 2: Producer sequence tracking ===\n";
    {
        Disruptor d;
        Order o{1, 0, 10000, 100, 0, Side::Bid, OrderType::Limit, OrderStatus::New};

        d.publish(o);
        std::cout << "  after 1 publish — producer seq: "
                  << d.producer_sequence() << " (expect 0)\n";

        Order out;
        d.consume(out);
        std::cout << "  after 1 consume — consumer seq: "
                  << d.consumer_sequence() << " (expect 0)\n";
        std::cout << "  consumed order id: " << out.order_id << " (expect 1)\n";
    }

    std::cout << "\n=== TEST 3: Buffer full detection ===\n";
    {
        Disruptor d;
        Order o{1, 0, 10000, 100, 0, Side::Bid, OrderType::Limit, OrderStatus::New};

        int64_t count = 0;
        while (d.publish(o)) count++;

        std::cout << "  slots filled before full: " << count
                  << " (expect 1024)\n";

        // Should reject now
        bool rejected = !d.publish(o);
        std::cout << "  next publish rejected: " << rejected
                  << " (expect 1)\n";
    }

    return 0;
}