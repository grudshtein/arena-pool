#include <array>
#include <chrono>

#include "arena_pool.hpp"
#include "bench_arena_pool.hpp"

static_assert(sizeof(int) == 4);

namespace bench {

template <typename T>
void warmup_pool(arena::arena_pool<T>& pool, const std::size_t ops = 1'000'000) {
    using namespace arena;
    std::vector<T*> ptrs;
    for (std::size_t i = 0; i != std::min(ops, pool.capacity()); ++i) {
        auto ptr = pool.allocate();
        (*ptr)[0] = -1; // touch data to avoid compiler optimization
        ptrs.push_back(ptr);
    }
    while (!ptrs.empty()) {
        pool.deallocate(ptrs.back());
        ptrs.pop_back();
    }
}

template <typename T>
void warmup_new_delete(const std::size_t ops = 1'000'000) {
    std::vector<T*> ptrs;
    for (std::size_t i = 0; i != ops; ++i) {
        auto ptr = new T;
        (*ptr)[0] = -1; // touch data to avoid compiler optimization
        ptrs.push_back(ptr);
    }
    while (!ptrs.empty()) {
        delete ptrs.back();
        ptrs.pop_back();
    }
}

template <typename T>
[[nodiscard]] Results run_batch(const Config& config) {
    using namespace arena;

    Results results{config};
    uint64_t op_count = 0;
    const auto capacity = config.capacity;
    arena_pool<T> pool(capacity);
    std::vector<T*> ptrs;
    ptrs.reserve(capacity);

    // time arena_pool
    warmup_pool<T>(pool);
    auto start = std::chrono::steady_clock::now();
    while (op_count < config.op_count) {
        for (std::size_t i = 0; i != capacity; ++i) {
            auto ptr = pool.allocate();
            (*ptr)[0] = -1; // touch data to avoid compiler optimization
            ptrs.push_back(ptr);
            ++op_count;
            if (op_count == config.op_count) break;
        }
        while (!ptrs.empty()) {
            pool.deallocate(ptrs.back());
            ptrs.pop_back();
        }
    }
    results.pool_time_ns = std::chrono::steady_clock::now() - start;

    // time new/delete
    op_count = 0;
    warmup_new_delete<T>();
    start = std::chrono::steady_clock::now();
    while (op_count < config.op_count) {
        for (std::size_t i = 0; i != capacity; ++i) {
            auto ptr = new T;
            (*ptr)[0] = -1; // touch data to avoid compiler optimization
            ptrs.push_back(ptr);
            ++op_count;
            if (op_count == config.op_count) break;
        }
        while (!ptrs.empty()) {
            delete ptrs.back();
            ptrs.pop_back();
        }
    }
    results.new_delete_time_ns = std::chrono::steady_clock::now() - start;

    return results;
}

template <typename T>
[[nodiscard]] Results run_rolling(const Config& config) {
    using namespace arena;

    Results results{config};
    uint64_t op_count = 0;
    const auto capacity = config.capacity;
    arena_pool<T> pool(capacity);
    std::vector<T*> ptrs;
    ptrs.reserve(capacity / 2);

    // time arena_pool
    warmup_pool<T>(pool);
    std::size_t ptr_idx = 0;
    for (std::size_t i = 0; i != capacity / 2; ++i) {
        ptrs.push_back(pool.allocate());
    }
    auto start = std::chrono::steady_clock::now();
    while (op_count++ < config.op_count) {
        auto ptr = pool.allocate();
        (*ptr)[0] = -1; // touch data to avoid compiler optimization
        pool.deallocate(ptrs[ptr_idx]);
        ptrs[ptr_idx] = ptr;
        ptr_idx = (ptr_idx + 1) % ptrs.size();
    }
    results.pool_time_ns = std::chrono::steady_clock::now() - start;
    while (!ptrs.empty()) {
        pool.deallocate(ptrs.back());
        ptrs.pop_back();
    }

    // time new/delete
    op_count = 0;
    warmup_new_delete<T>();
    ptr_idx = 0;
    for (std::size_t i = 0; i != capacity / 2; ++i) {
        ptrs.push_back(new T);
    }
    start = std::chrono::steady_clock::now();
    while (op_count++ < config.op_count) {
        auto ptr = new T;
        (*ptr)[0] = -1; // touch data to avoid compiler optimization
        delete ptrs[ptr_idx];
        ptrs[ptr_idx] = ptr;
        ptr_idx = (ptr_idx + 1) % ptrs.size();
    }
    results.new_delete_time_ns = std::chrono::steady_clock::now() - start;
    while (!ptrs.empty()) {
        delete ptrs.back();
        ptrs.pop_back();
    }

    return results;
}

Results run(const Config& config) {
    using small_type = std::array<int, 4>;
    using large_type = std::array<int, 32>;
    if (config.pattern == Config::Pattern::Batch) {
        return config.object_size == Config::ObjectSize::Small ? run_batch<small_type>(config)
                                                               : run_batch<large_type>(config);
    } else {
        return config.object_size == Config::ObjectSize::Small ? run_rolling<small_type>(config)
                                                               : run_rolling<large_type>(config);
    }
}

} // namespace bench
