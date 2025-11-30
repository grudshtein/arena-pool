#include <array>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "bench_arena_pool.hpp"

[[nodiscard]] std::ofstream open_csv(const std::string& relative_path, const std::string& header) {
    namespace fs = std::filesystem;
    fs::path path = fs::path(PROJECT_SOURCE_DIR) / relative_path;
    bool file_exists = fs::exists(path);
    std::ofstream csv_file(path, std::ios::app);
    if (!csv_file) throw std::runtime_error("Failed to open: " + path.string());
    if (!file_exists) csv_file << header << '\n';
    return csv_file;
}

void append_results(std::ofstream& csv_file, const bench::Results& results) {
    const auto pattern =
        results.config.pattern == bench::Config::Pattern::Batch ? "batch" : "rolling";
    const auto object_size =
        results.config.object_size == bench::Config::ObjectSize::Small ? "small" : "large";
    const auto data =
        std::format("{},{},{},{},{},{},{:.2f},{:.2f},{:.2f}", results.config.op_count,
                    results.config.capacity, pattern, object_size, results.pool_time_ns.count(),
                    results.new_delete_time_ns.count(), results.pool_op_latency_ns(),
                    results.new_delete_op_latency_ns(), results.pool_speedup_factor());
    csv_file << data << '\n';
}

int main() {
    const auto csv_path = "results//results.csv";
    const auto csv_header =
        "operations,capacity,pattern,object_size_profile,elapsed_pool_time_ns,elapsed_new_delete_"
        "time_ns,pool_op_latency_ns,new_delete_op_latency_ns,pool_speedup_factor";
    auto csv_file = open_csv(csv_path, csv_header);

    constexpr int num_repetitions = 5;
    constexpr uint64_t op_count = 100'000'000;
    constexpr std::array capacities{1'024, 8'192, 65'536, 262'144};
    constexpr std::array patterns{bench::Config::Pattern::Batch, bench::Config::Pattern::Rolling};
    constexpr std::array object_sizes{bench::Config::ObjectSize::Small,
                                      bench::Config::ObjectSize::Large};

    auto run_idx = 0;
    constexpr auto num_runs =
        capacities.size() * patterns.size() * object_sizes.size() * num_repetitions;
    for (const auto capacity : capacities) {
        for (const auto pattern : patterns) {
            for (const auto object_size : object_sizes) {
                for (auto i = 0; i != num_repetitions; ++i) {
                    std::cout << std::format("run {} of {}:\t", ++run_idx, num_runs);
                    const auto results = run(bench::Config{
                        op_count, static_cast<std::size_t>(capacity), pattern, object_size});
                    append_results(csv_file, results);
                    std::cout << std::format("arena pool speedup = {:.2f}x",
                                             results.pool_speedup_factor())
                              << std::endl;
                }
            }
        }
    }
    std::cout << "done" << std::endl;

    return 0;
}