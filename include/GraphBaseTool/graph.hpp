#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <bitset>
#include <chrono>
#include <nlohmann/json.hpp>

namespace routing {

// Хеш-функция для пар вершин
struct VertexPairHash {
    size_t operator()(const std::pair<size_t, size_t>& p) const {
        return std::hash<size_t>()(p.first) ^ (std::hash<size_t>()(p.second) << 1);
    }
};

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

struct PerformanceStats {
    std::vector<ProfilingResult> operation_times;
    size_t total_operations;
    size_t peak_memory_usage;
    double average_memory_usage;
};

struct GraphStats {
    size_t max_degree;
    double avg_degree;
    size_t connected_components;
    bool is_connected;
};

class Graph {
public:
    Graph() : vertex_count_(0) {}

    size_t add_vertex();
    void add_edge(const Edge& edge);
    void remove_edge(size_t from, size_t to);
    void update_edge_weight(size_t from, size_t to, double weight);
    std::vector<Edge> get_neighbors(size_t vertex) const;
    double get_edge_weight(size_t from, size_t to) const;
    size_t vertex_count() const;
    size_t edge_count() const;
    bool has_edge(size_t from, size_t to) const;

    // Алгоритмы поиска пути
    std::vector<double> dijkstra(size_t start) const;
    std::vector<size_t> a_star(size_t start, size_t goal,
                              std::function<double(size_t, size_t)> heuristic) const;
    std::vector<double> dijkstra_optimized(size_t start) const;
    std::vector<size_t> a_star_optimized(size_t start, size_t goal,
                                       std::function<double(size_t, size_t)> heuristic) const;

    // Алгоритм Беллмана-Форда
    std::pair<std::vector<double>, bool> bellman_ford(size_t start) const;

    // Алгоритм Флойда-Уоршелла
    std::vector<std::vector<double>> floyd_warshall() const;
    std::vector<std::vector<double>> floyd_warshall_parallel(size_t num_threads) const;
    std::vector<std::bitset<64>> floyd_warshall_bitset() const;

    // Поиск отрицательных циклов
    bool has_negative_cycle() const;
    std::vector<size_t> get_negative_cycle() const;

    // Загрузка и сохранение
    void load_from_csv(const std::string& filename);
    void load_from_json(const std::string& filename);
    std::string to_dot() const;

    // Кэширование
    void enable_caching(bool enable);
    void clear_cache();

    // Оптимизация
    void reserve(size_t vertices, size_t edges);

    // Статистика графа
    GraphStats get_stats() const;
    void update_stats() const;

    // Профилирование
    ProfilingResult profile_operation(const std::string& name, std::function<void()> operation) const;
    PerformanceStats get_performance_stats() const;
    void reset_performance_stats();

private:
    // Оптимизированная структура хранения графа
    struct Vertex {
        std::unordered_map<size_t, Edge> edges;
        size_t degree = 0;
    };
    
    std::vector<Vertex> vertices_;
    size_t vertex_count_;
    bool caching_enabled_ = false;
    
    // Оптимизированный кэш с LRU
    struct CacheEntry {
        double distance;
        std::chrono::steady_clock::time_point last_access;
    };
    mutable std::unordered_map<std::pair<size_t, size_t>, CacheEntry, VertexPairHash> distance_cache_;
    mutable std::unordered_map<std::pair<size_t, size_t>, std::vector<size_t>, VertexPairHash> path_cache_;
    
    mutable GraphStats stats_;
    mutable std::vector<ProfilingResult> profiling_results_;
    mutable size_t peak_memory_usage_ = 0;
    mutable size_t total_operations_ = 0;
    
    // Вспомогательные методы
    void update_cache_entry(const std::pair<size_t, size_t>& key, double distance) const;
    void cleanup_cache() const;
};

} // namespace routing 
