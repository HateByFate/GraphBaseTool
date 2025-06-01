#pragma once

#include <vector>
#include <map>
#include <functional>
#include <string>
#include <bitset>
#include <chrono>

namespace routing {

struct Edge {
    size_t from;
    size_t to;
    double weight;
};

struct ProfilingResult {
    std::string operation_name;
    std::chrono::nanoseconds duration;
    size_t memory_used;
};

class Graph {
public:
    Graph() : vertex_count_(0) {}

    size_t add_vertex();
    void add_edge(const Edge& edge);
    std::vector<Edge> get_neighbors(size_t vertex) const;
    double get_edge_weight(size_t from, size_t to) const;
    size_t vertex_count() const;
    size_t edge_count() const;

    std::vector<double> dijkstra(size_t start) const;
    std::vector<size_t> a_star(size_t start, size_t goal,
                              std::function<double(size_t, size_t)> heuristic) const;

    void load_from_csv(const std::string& filename);
    void load_from_json(const std::string& filename);
    std::string to_dot() const;

    void enable_caching(bool enable);
    void clear_cache();

    std::vector<double> dijkstra_optimized(size_t start) const;
    std::vector<size_t> a_star_optimized(size_t start, size_t goal,
                                       std::function<double(size_t, size_t)> heuristic) const;
    std::vector<std::vector<double>> floyd_warshall() const;
    std::vector<std::vector<double>> floyd_warshall_parallel(size_t num_threads) const;
    std::vector<std::bitset<64>> floyd_warshall_bitset() const;
    bool has_negative_cycle() const;
    std::vector<size_t> get_negative_cycle() const;
    void reserve(size_t vertices, size_t edges);

    struct GraphStats {
        size_t max_degree = 0;
        double avg_degree = 0;
        size_t connected_components = 0;
        bool is_connected = false;
    };
    GraphStats get_stats() const;
    void update_stats() const;

    struct PerformanceStats {
        std::vector<ProfilingResult> operation_times;
        size_t total_operations = 0;
        size_t peak_memory_usage = 0;
        double average_memory_usage = 0;
    };
    PerformanceStats get_performance_stats() const;
    void reset_performance_stats();
    ProfilingResult profile_operation(const std::string& name, std::function<void()> operation) const;

private:
    std::vector<std::vector<Edge>> adjacency_list_;
    std::map<size_t, std::map<size_t, Edge>> adjacency_map_;
    size_t vertex_count_;
    bool caching_enabled_ = false;
    mutable std::map<std::pair<size_t, size_t>, double> distance_cache_;
    mutable std::map<std::pair<size_t, size_t>, std::vector<size_t>> path_cache_;
    mutable GraphStats stats_;
    mutable std::vector<ProfilingResult> profiling_results_;
    mutable size_t peak_memory_usage_ = 0;
    mutable size_t total_operations_ = 0;
};

} // namespace routing 
