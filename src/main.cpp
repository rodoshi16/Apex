#include <iostream>
#include <vector>
#include "apex/order.hpp"
#include "apex/event.hpp"
#include "apex/order_book.hpp"

using namespace apex;

Order make_limit(uint64_t id, Side side, int64_t price, uint64_t qty) {
    return Order{id, 0, price, qty, 0, side, OrderType::Limit, OrderStatus::New};
}

Order make_market(uint64_t id, Side side, uint64_t qty) {
    return Order{id, 0, 0, qty, 0, side, OrderType::Market, OrderStatus::New};
}

void print_book(const OrderBook& book) {
    std::cout << "  best bid: ";
    if (book.has_bid()) std::cout << book.best_bid().value();
    else std::cout << "none";

    std::cout << " | best ask: ";
    if (book.has_ask()) std::cout << book.best_ask().value();
    else std::cout << "none";

    std::cout << " | spread: " << book.spread() << "\n";
    std::cout << "  bid depth: " << book.bid_depth();
    std::cout << " | ask depth: " << book.ask_depth() << "\n";
}

int main() {
    OrderBook book;
    std::vector<Event> events;

    std::cout << "=== TEST 1: Build the book ===\n";
    book.add_order(make_limit(1, Side::Bid, 9900, 100), events);
    book.add_order(make_limit(2, Side::Bid, 9950, 200), events);
    book.add_order(make_limit(3, Side::Ask, 10000, 150), events);
    book.add_order(make_limit(4, Side::Ask, 10050, 100), events);
    print_book(book);
    // best bid should be 9950, best ask 10000, spread 50

    std::cout << "\n=== TEST 2: Market buy order matches against best ask ===\n";
    events.clear();
    book.add_order(make_market(5, Side::Bid, 100), events);
    std::cout << "  events generated: " << events.size() << "\n";
    print_book(book);
    // ask at 10000 had 150, we bought 100, so 50 should remain

    std::cout << "\n=== TEST 3: Cancel a resting order ===\n";
    events.clear();
    book.cancel_order(2, events);
    std::cout << "  events generated: " << events.size() << "\n";
    print_book(book);
    // bid depth should drop by 200

    std::cout << "\n=== TEST 4: Limit order crosses the spread and matches ===\n";
    events.clear();
    book.add_order(make_limit(6, Side::Bid, 10000, 200), events);
    std::cout << "  events generated: " << events.size() << "\n";
    print_book(book);
    // should match against remaining 50 at ask 10000, then 150 rest on book as bid

    return 0;
}