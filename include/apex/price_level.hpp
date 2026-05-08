#pragma once
#include <cstdint>
#include <list>
#include <unordered_map>
#include <vector>
#include "apex/order.hpp"
#include "apex/event.hpp"

namespace apex {

class PriceLevel {
public:
    explicit PriceLevel(int64_t price) : price_(price) {}

    void     add_order(Order order);
    bool     cancel_order(uint64_t order_id);
    uint64_t fill(uint64_t quantity, std::vector<Event>& events_out);

    int64_t  price()          const { return price_; }
    uint64_t total_quantity() const { return total_quantity_; }
    bool     empty()          const { return orders_.empty(); }

private:
    int64_t  price_;
    uint64_t total_quantity_ = 0;

    std::list<Order> orders_;
    std::unordered_map<uint64_t, std::list<Order>::iterator> order_map_;
};

} // namespace apex