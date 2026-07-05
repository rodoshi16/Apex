#pragma once
#include <cstdint>
#include <cassert>
#include <array>
#include <atomic>

namespace apex {

template<typename T, std::size_t Capacity>
class ObjectPool {
public:
    ObjectPool() {
        for (std::size_t i = 0; i < Capacity - 1; ++i) {
            slots_[i].next = &slots_[i + 1];
        }
        slots_[Capacity - 1].next = nullptr;
        free_head_ = &slots_[0];
    }

    T* acquire() {
        Slot* slot = free_head_.load(std::memory_order_acquire);
        while (slot && !free_head_.compare_exchange_weak(
                slot, slot->next,
                std::memory_order_release,
                std::memory_order_acquire)) {}
        if (!slot) return nullptr;
        return reinterpret_cast<T*>(slot->storage);
    }

    void release(T* ptr) {
        ptr->~T();
        Slot* slot = reinterpret_cast<Slot*>(ptr);
        Slot* old_head = free_head_.load(std::memory_order_relaxed);
        do {
            slot->next = old_head;
        } while (!free_head_.compare_exchange_weak(
                old_head, slot,
                std::memory_order_release,
                std::memory_order_relaxed));
    }

    static constexpr std::size_t capacity() { return Capacity; }

private:
    struct alignas(T) Slot {
        std::byte storage[sizeof(T)];
        Slot* next = nullptr;
    };

    alignas(64) std::array<Slot, Capacity> slots_;
    alignas(64) std::atomic<Slot*> free_head_{nullptr};
};

} // namespace apex