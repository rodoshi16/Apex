#include <iostream>
#include <vector>
#include "apex/order.hpp"
#include "apex/event.hpp"
#include "apex/order_book.hpp"

using namespace apex;

Order make_limit(uint64_t id, Side side, int64_t price, uint64_t qty) {
    return Order{id, 0, price, qty, 0, side, OrderType::Limit, OrderStatus::New};
}

Order make_fok(uint64_t id, Side side, int64_t price, uint64_t qty) {
    return Order{id, 0, price, qty, 0, side, OrderType::FOK, OrderStatus::New};
}

void print_event(const Event& e) {
    const char* type = "unknown";
    switch (e.type) {
        case EventType::OrderAdded:         type = "OrderAdded";         break;
        case EventType::OrderCancelled:     type = "OrderCancelled";     break;
        case EventType::OrderFilled:        type = "OrderFilled";        break;
        case EventType::OrderPartiallyFilled: type = "PartialFilled";    break;
        case EventType::TradeExecuted:      type = "TradeExecuted";      break;
        default: break;
    }
    std::cout << "  [" << type << "] order=" << e.order_id
              << " qty=" << e.quantity << " price=" << e.price << "\n";
}

int main() {
    std::vector<Event> events;

    // TEST 1: FOK rejected — not enough liquidity
    std::cout << "=== TEST 1: FOK rejected — insufficient liquidity ===\n";
    {
        OrderBook book;
        book.add_order(make_limit(1, Side::Ask, 10000, 50), events);
        events.clear();

        // FOK wants 100 but only 50 available — should be rejected entirely
        book.add_order(make_fok(2, Side::Bid, 10000, 100), events);
        std::cout << "  events: " << events.size() << " (expect 1 — cancelled)\n";
        for (auto& e : events) print_event(e);
        std::cout << "  ask depth: " << book.ask_depth() << " (expect 50 — untouched)\n";
    }

    // TEST 2: FOK accepted — enough liquidity
    std::cout << "\n=== TEST 2: FOK accepted — sufficient liquidity ===\n";
    {
        OrderBook book;
        events.clear();
        book.add_order(make_limit(1, Side::Ask, 10000, 100), events);
        book.add_order(make_limit(2, Side::Ask, 10000, 100), events);
        events.clear();

        // FOK wants 150, 200 available — should fill completely
        book.add_order(make_fok(3, Side::Bid, 10000, 150), events);
        std::cout << "  events: " << events.size() << " (expect 2 — partial + filled)\n";
        for (auto& e : events) print_event(e);
        std::cout << "  ask depth: " << book.ask_depth() << " (expect 50 — 150 consumed)\n";
    }

    // TEST 3: FOK across multiple price levels
    std::cout << "\n=== TEST 3: FOK across multiple price levels ===\n";
    {
        OrderBook book;
        events.clear();
        book.add_order(make_limit(1, Side::Ask, 10000, 100), events);
        book.add_order(make_limit(2, Side::Ask, 10001, 100), events);
        events.clear();

        // FOK wants 200 across 2 levels — should fill completely
        book.add_order(make_fok(3, Side::Bid, 10001, 200), events);
        std::cout << "  events: " << events.size() << " (expect 2 — both filled)\n";
        for (auto& e : events) print_event(e);
        std::cout << "  ask depth: " << book.ask_depth() << " (expect 0 — fully consumed)\n";
    }

    return 0;
}