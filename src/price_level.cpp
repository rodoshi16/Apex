#include "apex/price_level.hpp"

namespace apex {

void PriceLevel::add_order(Order order) {
    total_quantity_ += order.remaining_quantity();
    orders_.push_back(order);
    order_map_[order.order_id] = std::prev(orders_.end());
}

bool PriceLevel::cancel_order(uint64_t order_id) {
    auto it = order_map_.find(order_id);
    if (it == order_map_.end()) return false;

    total_quantity_ -= it->second->remaining_quantity();
    orders_.erase(it->second);
    order_map_.erase(it);
    return true;
}

uint64_t PriceLevel::fill(uint64_t quantity, std::vector<Event>& events_out) {
    uint64_t remaining = quantity;

    while (!orders_.empty() && remaining > 0) {
        Order& front = orders_.front();
        uint64_t fill_qty = std::min(remaining, front.remaining_quantity());

        front.filled_quantity += fill_qty;
        total_quantity_       -= fill_qty;
        remaining             -= fill_qty;

        EventType etype = front.remaining_quantity() == 0
            ? EventType::OrderFilled
            : EventType::OrderPartiallyFilled;

        events_out.push_back(Event{
            etype,
            0,
            front.order_id,
            0,
            price_,
            fill_qty,
            front.side
        });

        if (front.remaining_quantity() == 0) {
            order_map_.erase(front.order_id);
            orders_.pop_front();
        }
    }

    return quantity - remaining;
}

} // namespace apex