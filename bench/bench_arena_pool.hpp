#pragma once

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace bench {

struct Config {
    enum class Pattern { Batch, Rolling };

    enum class ObjectSize { Small, Large };

    uint64_t op_count; // iterations of each operation (allocate, deallocate, new, delete)
    std::size_t capacity{65'536};              // arena_pool capacity
    Pattern pattern{Pattern::Batch};           // full-batch cycles or steady-state rolling
    ObjectSize object_size{ObjectSize::Small}; // object size profile
};

struct Results {
    Config config;
    std::chrono::nanoseconds pool_time_ns{0};
    std::chrono::nanoseconds new_delete_time_ns{0};

    [[nodiscard]] double pool_op_latency_ns() const {
        return static_cast<double>(pool_time_ns.count()) / static_cast<double>(config.op_count * 2);
    }

    [[nodiscard]] double new_delete_op_latency_ns() const {
        return static_cast<double>(new_delete_time_ns.count()) /
               static_cast<double>(config.op_count * 2);
    }

    [[nodiscard]] double pool_speedup_factor() const {
        const double pool_t = static_cast<double>(pool_time_ns.count());
        const double new_delete_t = static_cast<double>(new_delete_time_ns.count());
        return new_delete_t / pool_t;
    }
};

[[nodiscard]] Results run(const Config& config);

} // namespace bench