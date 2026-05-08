#include <iostream>
#include "apex/order.hpp"

int main() {
    apex::Order order{
        1,                          // order_id
        0,                          // timestamp_ns
        10000,                      // price (basis points — $1.00)
        100,                        // quantity
        0,                          // filled_quantity
        apex::Side::Bid,            // side
        apex::OrderType::Limit,     // type
        apex::OrderStatus::New      // status
    };

    std::cout << "apex online\n";
    std::cout << "order id:    " << order.order_id << "\n";
    std::cout << "price:       " << order.price << " bps\n";
    std::cout << "quantity:    " << order.quantity << "\n";
    std::cout << "remaining:   " << order.remaining_quantity() << "\n";
    std::cout << "is buy:      " << order.is_buy() << "\n";

    return 0;
}