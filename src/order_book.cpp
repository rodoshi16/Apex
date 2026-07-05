#include "apex/order_book.hpp"

namespace apex {

std::optional<int64_t> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<int64_t> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

uint64_t OrderBook::bid_depth() const {
    uint64_t total = 0;
    for (const auto& [price, level] : bids_) total += level.total_quantity();
    return total;
}

uint64_t OrderBook::ask_depth() const {
    uint64_t total = 0;
    for (const auto& [price, level] : asks_) total += level.total_quantity();
    return total;
}

bool OrderBook::can_fill(const Order& order) const {
    uint64_t available = 0;

    if (order.is_buy()) {
        for (const auto& [price, level] : asks_) {
            if (order.price != 0 && price > order.price) break;
            available += level.total_quantity();
            if (available >= order.quantity) return true;
        }
    } else {
        for (const auto& [price, level] : bids_) {
            if (order.price != 0 && price < order.price) break;
            available += level.total_quantity();
            if (available >= order.quantity) return true;
        }
    }

    return false;
}

void OrderBook::match(Order& incoming, std::vector<Event>& events_out) {
    bool is_market = incoming.type == OrderType::Market;

    if (incoming.is_buy()) {
        while (!asks_.empty() && incoming.remaining_quantity() > 0) {
            auto it = asks_.begin();
            // market orders match at any price, limit orders must cross
            if (!is_market && incoming.price < it->first) break;

            uint64_t filled = it->second.fill(incoming.remaining_quantity(), events_out);
            incoming.filled_quantity += filled;

            if (it->second.empty()) asks_.erase(it);
        }
    } else {
        while (!bids_.empty() && incoming.remaining_quantity() > 0) {
            auto it = bids_.begin();
            // market orders match at any price, limit orders must cross
            if (!is_market && incoming.price > it->first) break;

            uint64_t filled = it->second.fill(incoming.remaining_quantity(), events_out);
            incoming.filled_quantity += filled;

            if (it->second.empty()) bids_.erase(it);
        }
    }
}

void OrderBook::add_order(Order order, std::vector<Event>& events_out) {
    // FOK: check if full fill is possible before touching anything
    if (order.type == OrderType::FOK) {
        if (!can_fill(order)) {
            events_out.push_back(Event{
                EventType::OrderCancelled,
                order.timestamp_ns,
                order.order_id,
                0,
                order.price,
                order.quantity,
                order.side
            });
            return;
        }
    }

    // Try to match first
    match(order, events_out);

    // If limit order with remaining quantity, rest on the book
    if (order.type == OrderType::Limit && order.remaining_quantity() > 0) {
        if (order.is_buy()) {
            if (bids_.find(order.price) == bids_.end()) {
                bids_.emplace(order.price, PriceLevel(order.price));
            }
            bids_.at(order.price).add_order(order);
        } else {
            if (asks_.find(order.price) == asks_.end()) {
                asks_.emplace(order.price, PriceLevel(order.price));
            }
            asks_.at(order.price).add_order(order);
        }

        order_locations_[order.order_id] = {order.side, order.price};

        events_out.push_back(Event{
            EventType::OrderAdded,
            order.timestamp_ns,
            order.order_id,
            0,
            order.price,
            order.remaining_quantity(),
            order.side
        });
    }

    // IOC — cancel remaining
    if (order.type == OrderType::IOC && order.remaining_quantity() > 0) {
        events_out.push_back(Event{
            EventType::OrderCancelled,
            order.timestamp_ns,
            order.order_id,
            0,
            order.price,
            order.remaining_quantity(),
            order.side
        });
    }

// FOK — handled before matching, so nothing to do here
// FOK orders are rejected entirely in add_order if can_fill returns false
}

void OrderBook::cancel_order(uint64_t order_id, std::vector<Event>& events_out) {
    auto loc_it = order_locations_.find(order_id);
    if (loc_it == order_locations_.end()) return;

    Side    side  = loc_it->second.side;
    int64_t price = loc_it->second.price;

    if (side == Side::Bid) {
        auto level_it = bids_.find(price);
        if (level_it != bids_.end()) {
            level_it->second.cancel_order(order_id);
            if (level_it->second.empty()) bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(price);
        if (level_it != asks_.end()) {
            level_it->second.cancel_order(order_id);
            if (level_it->second.empty()) asks_.erase(level_it);
        }
    }

    events_out.push_back(Event{
        EventType::OrderCancelled,
        0,
        order_id,
        0,
        price,
        0,
        side
    });

    order_locations_.erase(loc_it);
}

} // namespace apex