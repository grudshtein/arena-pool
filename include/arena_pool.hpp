#pragma once

#include <cassert>
#include <vector>

template <typename T> class arena_pool {
  public:
    arena_pool(const std::size_t capacity) : capacity_{capacity} {
        arena_ = new T[capacity_];
        pool_.reserve(capacity_);
        for (std::size_t i = 0; i != capacity_; ++i) {
            pool_.push_back(&arena_[i]);
        }
        allocated_.resize(capacity_, false);
    }

    arena_pool(const arena_pool& other) = delete;
    arena_pool(arena_pool&& other) = delete;
    arena_pool& operator=(const arena_pool& other) = delete;
    arena_pool& operator=(arena_pool&& other) = delete;

    ~arena_pool() {
        delete[] arena_;
    }

    [[nodiscard]] T* allocate() noexcept {
        if (empty()) return nullptr;
        auto ptr = pool_.back();
        pool_.pop_back();
        allocated(ptr) = true;
        return ptr;
    }

    void deallocate(T* ptr) noexcept {
        if (!ptr) {
            assert(false && "attempted to deallocate nullptr");
            return;
        }
        if (ptr < arena_ || ptr >= arena_ + capacity_) {
            assert(false && "attempted to deallocate foreign ptr");
            return;
        }
        if (!allocated(ptr)) {
            assert(false && "attempted double deallocate");
            return;
        }
        allocated(ptr) = false;
        pool_.push_back(ptr);
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] std::size_t used() const noexcept {
        return capacity_ - pool_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return pool_.empty();
    }

    [[nodiscard]] bool full() const noexcept {
        return pool_.size() == capacity_;
    }

  private:
    const std::size_t capacity_;
    T* arena_;
    std::vector<T*> pool_;
    std::vector<uint8_t> allocated_;

    [[nodiscard]] uint8_t& allocated(T* ptr) noexcept {
        return allocated_[static_cast<std::size_t>(ptr - arena_)];
    }
};