#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif

#include "../include/GraphBaseTool/graph.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>
#include <algorithm>
#include <chrono>
#include <thread>
#include <bitset>
#include <stack>

using json = nlohmann::json;

namespace routing {

size_t Graph::add_vertex() {
    vertices_.emplace_back();
    return vertex_count_++;
}

void Graph::add_edge(const Edge& edge) {
    if (edge.from >= vertex_count_ || edge.to >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    vertices_[edge.from].edges[edge.to] = edge;
    vertices_[edge.from].degree++;
    update_stats();
}

void Graph::remove_edge(size_t from, size_t to) {
    if (from >= vertex_count_ || to >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    auto& edges = vertices_[from].edges;
    auto it = edges.find(to);
    if (it != edges.end()) {
        edges.erase(it);
        vertices_[from].degree--;
        update_stats();
    }
}

void Graph::update_edge_weight(size_t from, size_t to, double weight) {
    if (from >= vertex_count_ || to >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    auto& edges = vertices_[from].edges;
    auto it = edges.find(to);
    if (it != edges.end()) {
        it->second.weight = weight;
    }
}

std::vector<Edge> Graph::get_neighbors(size_t vertex) const {
    if (vertex >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    std::vector<Edge> neighbors;
    neighbors.reserve(vertices_[vertex].edges.size());
    for (const auto& pair : vertices_[vertex].edges) {
        auto to = pair.first;
        const auto& edge = pair.second;
        neighbors.push_back(edge);
    }
    return neighbors;
}

double Graph::get_edge_weight(size_t from, size_t to) const {
    if (from >= vertex_count_ || to >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    const auto& edges = vertices_[from].edges;
    auto it = edges.find(to);
    return it != edges.end() ? it->second.weight : std::numeric_limits<double>::infinity();
}

size_t Graph::vertex_count() const {
    return vertex_count_;
}

size_t Graph::edge_count() const {
    size_t count = 0;
    for (const auto& vertex : vertices_) {
        count += vertex.edges.size();
    }
    return count;
}

bool Graph::has_edge(size_t from, size_t to) const {
    if (from >= vertices_.size() || to >= vertices_.size()) {
        return false;
    }
    return vertices_[from].edges.find(to) != vertices_[from].edges.end();
}

std::vector<double> Graph::dijkstra(size_t start) const {
    if (start >= vertex_count_) {
        throw std::out_of_range("Start vertex index out of range");
    }
    std::vector<double> distances(vertex_count_, std::numeric_limits<double>::infinity());
    distances[start] = 0;
    std::priority_queue<std::pair<double, size_t>,
                       std::vector<std::pair<double, size_t>>,
                       std::greater<>> pq;
    pq.push({0, start});
    while (!pq.empty()) {
        auto [dist, u] = pq.top();
        pq.pop();
        if (dist > distances[u]) continue;
        for (const auto& pair : vertices_[u].edges) {
            auto v = pair.first;
            const auto& edge = pair.second;
            double new_dist = distances[u] + edge.weight;
            if (new_dist < distances[v]) {
                distances[v] = new_dist;
                pq.push({new_dist, v});
            }
        }
    }
    return distances;
}

std::vector<size_t> Graph::a_star(size_t start, size_t goal,
                                 std::function<double(size_t, size_t)> heuristic) const {
    if (start >= vertex_count_ || goal >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    std::vector<double> g_score(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<double> f_score(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<size_t> came_from(vertex_count_, vertex_count_);
    g_score[start] = 0;
    f_score[start] = heuristic(start, goal);
    std::priority_queue<std::pair<double, size_t>,
                       std::vector<std::pair<double, size_t>>,
                       std::greater<>> open_set;
    open_set.push({f_score[start], start});
    while (!open_set.empty()) {
        auto [current_f, current] = open_set.top();
        open_set.pop();
        if (current == goal) {
            std::vector<size_t> path;
            for (size_t v = current; v != vertex_count_; v = came_from[v]) {
                path.push_back(v);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        for (const auto& pair : vertices_[current].edges) {
            auto neighbor = pair.first;
            const auto& edge = pair.second;
            double tentative_g_score = g_score[current] + edge.weight;
            if (tentative_g_score < g_score[neighbor]) {
                came_from[neighbor] = current;
                g_score[neighbor] = tentative_g_score;
                f_score[neighbor] = g_score[neighbor] + heuristic(neighbor, goal);
                open_set.push({f_score[neighbor], neighbor});
            }
        }
    }
    return {}; // Путь не найден
}

std::vector<double> Graph::dijkstra_optimized(size_t start) const {
    if (start >= vertex_count_) {
        throw std::out_of_range("Start vertex index out of range");
    }
    std::vector<double> distances(vertex_count_, std::numeric_limits<double>::infinity());
    distances[start] = 0;
    std::vector<bool> visited(vertex_count_, false);
    std::priority_queue<std::pair<double, size_t>,
                       std::vector<std::pair<double, size_t>>,
                       std::greater<>> pq;
    pq.push({0, start});
    while (!pq.empty()) {
        auto [dist, u] = pq.top();
        pq.pop();
        if (visited[u]) continue;
        visited[u] = true;
        for (const auto& pair : vertices_[u].edges) {
            auto v = pair.first;
            const auto& edge = pair.second;
            if (!visited[v]) {
                double new_dist = distances[u] + edge.weight;
                if (new_dist < distances[v]) {
                    distances[v] = new_dist;
                    pq.push({new_dist, v});
                }
            }
        }
    }
    return distances;
}

std::vector<size_t> Graph::a_star_optimized(size_t start, size_t goal,
                                          std::function<double(size_t, size_t)> heuristic) const {
    if (start >= vertex_count_ || goal >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    std::vector<double> g_score(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<double> f_score(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<size_t> came_from(vertex_count_, vertex_count_);
    std::vector<bool> closed_set(vertex_count_, false);
    g_score[start] = 0;
    f_score[start] = heuristic(start, goal);
    std::priority_queue<std::pair<double, size_t>,
                       std::vector<std::pair<double, size_t>>,
                       std::greater<>> open_set;
    open_set.push({f_score[start], start});
    while (!open_set.empty()) {
        auto [current_f, current] = open_set.top();
        open_set.pop();
        if (current == goal) {
            std::vector<size_t> path;
            for (size_t v = current; v != vertex_count_; v = came_from[v]) {
                path.push_back(v);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        if (closed_set[current]) continue;
        closed_set[current] = true;
        for (const auto& pair : vertices_[current].edges) {
            auto neighbor = pair.first;
            const auto& edge = pair.second;
            if (closed_set[neighbor]) continue;
            double tentative_g_score = g_score[current] + edge.weight;
            if (tentative_g_score < g_score[neighbor]) {
                came_from[neighbor] = current;
                g_score[neighbor] = tentative_g_score;
                f_score[neighbor] = g_score[neighbor] + heuristic(neighbor, goal);
                open_set.push({f_score[neighbor], neighbor});
            }
        }
    }
    return {}; // Путь не найден
}

std::pair<std::vector<double>, bool> Graph::bellman_ford(size_t start) const {
    if (start >= vertex_count_) {
        throw std::out_of_range("Start vertex index out of range");
    }
    std::vector<double> distances(vertex_count_, std::numeric_limits<double>::infinity());
    distances[start] = 0;
    for (size_t i = 0; i < vertex_count_ - 1; ++i) {
        for (size_t u = 0; u < vertex_count_; ++u) {
            for (const auto& pair : vertices_[u].edges) {
                auto v = pair.first;
                const auto& edge = pair.second;
                if (distances[u] != std::numeric_limits<double>::infinity() &&
                    distances[u] + edge.weight < distances[v]) {
                    distances[v] = distances[u] + edge.weight;
                }
            }
        }
    }
    for (size_t u = 0; u < vertex_count_; ++u) {
        for (const auto& pair : vertices_[u].edges) {
            auto v = pair.first;
            const auto& edge = pair.second;
            if (distances[u] != std::numeric_limits<double>::infinity() &&
                distances[u] + edge.weight < distances[v]) {
                return {distances, true};
            }
        }
    }
    return {distances, false};
}

std::vector<std::vector<double>> Graph::floyd_warshall() const {
    std::vector<std::vector<double>> dist(vertex_count_,
                                        std::vector<double>(vertex_count_, std::numeric_limits<double>::infinity()));
    for (size_t i = 0; i < vertex_count_; ++i) {
        dist[i][i] = 0;
    }
    for (size_t i = 0; i < vertex_count_; ++i) {
        for (const auto& pair : vertices_[i].edges) {
            auto j = pair.first;
            const auto& edge = pair.second;
            dist[i][j] = edge.weight;
        }
    }
    for (size_t k = 0; k < vertex_count_; ++k) {
        for (size_t i = 0; i < vertex_count_; ++i) {
            for (size_t j = 0; j < vertex_count_; ++j) {
                if (dist[i][k] != std::numeric_limits<double>::infinity() &&
                    dist[k][j] != std::numeric_limits<double>::infinity()) {
                    dist[i][j] = std::min(dist[i][j], dist[i][k] + dist[k][j]);
                }
            }
        }
    }
    return dist;
}

std::vector<std::vector<double>> Graph::floyd_warshall_parallel(size_t num_threads) const {
    std::vector<std::vector<double>> dist(vertex_count_,
                                        std::vector<double>(vertex_count_, std::numeric_limits<double>::infinity()));
    for (size_t i = 0; i < vertex_count_; ++i) {
        dist[i][i] = 0;
    }
    for (size_t i = 0; i < vertex_count_; ++i) {
        for (const auto& pair : vertices_[i].edges) {
            auto j = pair.first;
            const auto& edge = pair.second;
            dist[i][j] = edge.weight;
        }
    }
    std::vector<std::thread> threads;
    for (size_t k = 0; k < vertex_count_; ++k) {
        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, k, t]() {
                size_t chunk_size = vertex_count_ / num_threads;
                size_t start = t * chunk_size;
                size_t end = (t == num_threads - 1) ? vertex_count_ : (t + 1) * chunk_size;
                for (size_t i = start; i < end; ++i) {
                    for (size_t j = 0; j < vertex_count_; ++j) {
                        if (dist[i][k] != std::numeric_limits<double>::infinity() &&
                            dist[k][j] != std::numeric_limits<double>::infinity()) {
                            dist[i][j] = std::min(dist[i][j], dist[i][k] + dist[k][j]);
                        }
                    }
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
    return dist;
}

std::vector<std::bitset<64>> Graph::floyd_warshall_bitset() const {
    std::vector<std::bitset<64>> dist(vertex_count_);
    for (size_t i = 0; i < vertex_count_; ++i) {
        for (const auto& pair : vertices_[i].edges) {
            auto j = pair.first;
            const auto& edge = pair.second;
            dist[i].set(j);
        }
    }
    for (size_t k = 0; k < vertex_count_; ++k) {
        for (size_t i = 0; i < vertex_count_; ++i) {
            if (dist[i].test(k)) {
                dist[i] |= dist[k];
            }
        }
    }
    return dist;
}

bool Graph::has_negative_cycle() const {
    auto [distances, has_negative_cycle] = bellman_ford(0);
    return has_negative_cycle;
}

std::vector<size_t> Graph::get_negative_cycle() const {
    std::vector<double> distances(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<size_t> predecessor(vertex_count_, vertex_count_);
    distances[0] = 0;
    size_t cycle_start = vertex_count_;
    for (size_t i = 0; i < vertex_count_; ++i) {
        for (size_t u = 0; u < vertex_count_; ++u) {
            for (const auto& pair : vertices_[u].edges) {
                auto v = pair.first;
                const auto& edge = pair.second;
                if (distances[u] != std::numeric_limits<double>::infinity() &&
                    distances[u] + edge.weight < distances[v]) {
                    distances[v] = distances[u] + edge.weight;
                    predecessor[v] = u;
                    if (i == vertex_count_ - 1) {
                        cycle_start = v;
                    }
                }
            }
        }
    }
    if (cycle_start == vertex_count_) {
        return {}; // Отрицательный цикл не найден
    }
    std::vector<bool> visited(vertex_count_, false);
    std::vector<size_t> cycle;
    size_t current = cycle_start;
    while (!visited[current]) {
        visited[current] = true;
        cycle.push_back(current);
        current = predecessor[current];
    }
    auto it = std::find(cycle.begin(), cycle.end(), current);
    cycle.erase(cycle.begin(), it);
    std::reverse(cycle.begin(), cycle.end());
    return cycle;
}

void Graph::load_from_csv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        size_t from, to;
        double weight;
        if (iss >> from >> to >> weight) {
            Edge edge{from, to, weight};
            add_edge(edge);
        }
    }
}

void Graph::load_from_json(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    json j;
    file >> j;
    for (const auto& edge : j["edges"]) {
        Edge e{edge["from"], edge["to"], edge["weight"]};
        add_edge(e);
    }
}

std::string Graph::to_dot() const {
    std::stringstream ss;
    ss << "digraph G {\n";
    for (size_t i = 0; i < vertex_count_; ++i) {
        ss << "  " << i << ";\n";
    }
    for (size_t i = 0; i < vertex_count_; ++i) {
        for (const auto& pair : vertices_[i].edges) {
            auto to = pair.first;
            const auto& edge = pair.second;
            ss << "  " << i << " -> " << to << " [label=\"" << edge.weight << "\"];\n";
        }
    }
    ss << "}";
    return ss.str();
}

void Graph::enable_caching(bool enable) {
    caching_enabled_ = enable;
    if (!enable) {
        clear_cache();
    }
}

void Graph::clear_cache() {
    distance_cache_.clear();
    path_cache_.clear();
}

void Graph::reserve(size_t vertices, size_t edges) {
    vertices_.reserve(vertices);
    for (auto& vertex : vertices_) {
        vertex.edges.reserve(edges / vertices);
    }
}

GraphStats Graph::get_stats() const {
    return stats_;
}

void Graph::update_stats() const {
    stats_.max_degree = 0;
    stats_.avg_degree = 0;
    stats_.connected_components = 0;
    stats_.is_connected = false;
    size_t total_degree = 0;
    for (const auto& vertex : vertices_) {
        stats_.max_degree = std::max(stats_.max_degree, vertex.degree);
        total_degree += vertex.degree;
    }
    stats_.avg_degree = static_cast<double>(total_degree) / vertex_count_;
    std::vector<bool> visited(vertex_count_, false);
    std::function<void(size_t)> dfs = [&](size_t v) {
        visited[v] = true;
        for (const auto& pair : vertices_[v].edges) {
            auto u = pair.first;
            if (!visited[u]) {
                dfs(u);
            }
        }
    };
    dfs(0);
    stats_.is_connected = std::all_of(visited.begin(), visited.end(), [](bool v) { return v; });
    std::fill(visited.begin(), visited.end(), false);
    for (size_t i = 0; i < vertex_count_; ++i) {
        if (!visited[i]) {
            dfs(i);
            ++stats_.connected_components;
        }
    }
}

ProfilingResult Graph::profile_operation(const std::string& name, std::function<void()> operation) const {
    auto start_time = std::chrono::steady_clock::now();
    size_t start_memory = peak_memory_usage_;
    operation();
    auto end_time = std::chrono::steady_clock::now();
    size_t end_memory = peak_memory_usage_;
    ProfilingResult result{
        name,
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time),
        end_memory - start_memory
    };
    profiling_results_.push_back(result);
    ++total_operations_;
    peak_memory_usage_ = std::max(peak_memory_usage_, end_memory);
    return result;
}

PerformanceStats Graph::get_performance_stats() const {
    PerformanceStats stats;
    stats.operation_times = profiling_results_;
    stats.total_operations = total_operations_;
    stats.peak_memory_usage = peak_memory_usage_;
    if (!profiling_results_.empty()) {
        double total_duration = 0;
        for (const auto& result : profiling_results_) {
            total_duration += std::chrono::duration<double>(result.duration).count();
        }
        stats.average_memory_usage = total_duration / profiling_results_.size();
    }
    return stats;
}

void Graph::reset_performance_stats() {
    profiling_results_.clear();
    total_operations_ = 0;
    peak_memory_usage_ = 0;
}

void Graph::update_cache_entry(const std::pair<size_t, size_t>& key, double distance) const {
    if (!caching_enabled_) return;
    auto now = std::chrono::steady_clock::now();
    distance_cache_[key] = {distance, now};
    cleanup_cache();
}

void Graph::cleanup_cache() const {
    if (!caching_enabled_) return;
    auto now = std::chrono::steady_clock::now();
    auto threshold = now - std::chrono::minutes(5);
    for (auto it = distance_cache_.begin(); it != distance_cache_.end();) {
        if (it->second.last_access < threshold) {
            it = distance_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace routing


