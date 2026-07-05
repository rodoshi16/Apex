#pragma once
#include <cstdint>
#include <atomic>
#include <array>
#include <cassert>
#include "apex/order.hpp"

namespace apex {

static constexpr int64_t RING_BUFFER_SIZE = 1024;
static_assert((RING_BUFFER_SIZE & (RING_BUFFER_SIZE - 1)) == 0,
              "Ring buffer size must be a power of two");

struct alignas(64) Slot {
    Order    order;
    uint64_t sequence;
};

class Disruptor {
public:
    Disruptor() {
        producer_sequence_.store(-1, std::memory_order_relaxed);
        consumer_sequence_.store(-1, std::memory_order_relaxed);
    }

    // Producer: claim the next slot and write an order into it
    // Returns false if the buffer is full
    bool publish(const Order& order) {
        int64_t next = producer_sequence_.load(std::memory_order_relaxed) + 1;

        // Check if buffer is full
        // Consumer must have processed slot (next - RING_BUFFER_SIZE)
        int64_t wrap_point = next - RING_BUFFER_SIZE;
        if (consumer_sequence_.load(std::memory_order_acquire) < wrap_point) {
            return false; // buffer full
        }

        // Write the order into the slot
        int64_t index = next & (RING_BUFFER_SIZE - 1); // fast modulo
        buffer_[index].order    = order;
        buffer_[index].sequence = next;

        // Publish — consumer can now see this slot
        producer_sequence_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer: try to read the next unconsumed slot
    // Returns false if no new slots are available
    bool consume(Order& order_out) {
        int64_t next = consumer_sequence_.load(std::memory_order_relaxed) + 1;

        // Check if producer has published this slot yet
        if (producer_sequence_.load(std::memory_order_acquire) < next) {
            return false; // nothing to consume
        }

        int64_t index = next & (RING_BUFFER_SIZE - 1);
        order_out = buffer_[index].order;

        // Advance consumer sequence
        consumer_sequence_.store(next, std::memory_order_release);
        return true;
    }

    int64_t producer_sequence() const {
        return producer_sequence_.load(std::memory_order_relaxed);
    }

    int64_t consumer_sequence() const {
        return consumer_sequence_.load(std::memory_order_relaxed);
    }

private:
    // Each on its own cache line — prevents false sharing
    alignas(64) std::atomic<int64_t> producer_sequence_;
    alignas(64) std::atomic<int64_t> consumer_sequence_;
    alignas(64) std::array<Slot, RING_BUFFER_SIZE> buffer_;
};

} // namespace apex