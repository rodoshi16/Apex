#include <iostream>
#include <vector>
#include "apex/order.hpp"
#include "apex/event.hpp"
#include "apex/price_level.hpp"

using namespace apex;

Order make_order(uint64_t id, uint64_t qty, Side side) {
    return Order{id, 0, 10000, qty, 0, side, OrderType::Limit, OrderStatus::New};
}

int main() {
    PriceLevel level(10000);

    // Add three orders
    level.add_order(make_order(1, 100, Side::Bid));
    level.add_order(make_order(2, 200, Side::Bid));
    level.add_order(make_order(3, 150, Side::Bid));

    std::cout << "total quantity: " << level.total_quantity() << "\n"; // should be 450

    // Cancel order 2
    level.cancel_order(2);
    std::cout << "after cancel:   " << level.total_quantity() << "\n"; // should be 250

    // Fill 100 — should hit order 1 first (FIFO)
    std::vector<Event> events;
    uint64_t filled = level.fill(100, events);

    std::cout << "filled:         " << filled << "\n";          // should be 100
    std::cout << "events:         " << events.size() << "\n";   // should be 1
    std::cout << "filled order:   " << events[0].order_id << "\n"; // should be 1
    std::cout << "remaining:      " << level.total_quantity() << "\n"; // should be 150

    return 0;
}