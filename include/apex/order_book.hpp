#pragma once
#include <map>
#include <functional>
#include <unordered_map>
#include <vector>
#include <optional>
#include "apex/order.hpp"
#include "apex/event.hpp"
#include "apex/price_level.hpp"

namespace apex {

class OrderBook {
public:
    void add_order(Order order, std::vector<Event>& events_out);
    void cancel_order(uint64_t order_id, std::vector<Event>& events_out);

    std::optional<int64_t> best_bid() const;
    std::optional<int64_t> best_ask() const;

    bool has_bid() const { return !bids_.empty(); }
    bool has_ask() const { return !asks_.empty(); }

    int64_t spread() const {
        if (!has_bid() || !has_ask()) return -1;
        return best_ask().value() - best_bid().value();
    }

    uint64_t bid_depth() const;
    uint64_t ask_depth() const;

private:
    // Bids: highest price first
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;

    // Asks: lowest price first
    std::map<int64_t, PriceLevel> asks_;

    // Order ID → which side and price it's at
    struct OrderLocation {
        Side    side;
        int64_t price;
    };
    std::unordered_map<uint64_t, OrderLocation> order_locations_;

    void match(Order& incoming, std::vector<Event>& events_out);
};

} // namespace apex