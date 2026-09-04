#pragma once
#include <cstdint>

namespace apex {

enum class Side : uint8_t {
    Bid, 
    Ask 
};

enum class OrderType : uint8_t {
    Limit,
    Market,
    IOC,
    FOK
};

enum class OrderStatus : uint8_t {
    New,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected
};

struct Order {
    uint64_t    order_id;
    uint64_t    timestamp_ns;
    int64_t     price;
    uint64_t    quantity;
    uint64_t    filled_quantity;
    Side        side;
    OrderType   type;
    OrderStatus status;

    uint64_t remaining_quantity() const {
        return quantity - filled_quantity;
    }

    bool is_buy()  const { return side == Side::Bid; }
    bool is_sell() const { return side == Side::Ask; }
};

}
