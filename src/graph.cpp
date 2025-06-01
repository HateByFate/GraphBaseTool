#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif

#include <GraphBaseTool/graph.hpp>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>
#include <algorithm>
#include "../include/nlohmann/json.hpp"
#include <chrono>
#include <thread>
#include <bitset>
#include <stack>

using json = nlohmann::json;

namespace routing {

size_t Graph::add_vertex() {
    adjacency_list_.push_back({});
    return vertex_count_++;
}

void Graph::add_edge(const Edge& edge) {
    if (edge.from >= vertex_count_ || edge.to >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    adjacency_list_[edge.from].push_back(edge);
    adjacency_map_[edge.from][edge.to] = edge;
}

std::vector<Edge> Graph::get_neighbors(size_t vertex) const {
    if (vertex >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    return adjacency_list_[vertex];
}

double Graph::get_edge_weight(size_t from, size_t to) const {
    if (from >= vertex_count_ || to >= vertex_count_) {
        return std::numeric_limits<double>::infinity();
    }
    
    auto it = adjacency_map_.find(from);
    if (it != adjacency_map_.end()) {
        auto edge_it = it->second.find(to);
        if (edge_it != it->second.end()) {
            return edge_it->second.weight;
        }
    }
    
    return std::numeric_limits<double>::infinity();
}

size_t Graph::vertex_count() const {
    return vertex_count_;
}

size_t Graph::edge_count() const {
    size_t count = 0;
    for (const auto& edges : adjacency_list_) {
        count += edges.size();
    }
    return count;
}

std::vector<double> Graph::dijkstra(size_t start) const {
    if (start >= vertex_count_) {
        throw std::out_of_range("Start vertex index out of range");
    }

    // Проверка на отрицательные веса
    for (const auto& edges : adjacency_list_) {
        for (const auto& edge : edges) {
            if (edge.weight < 0) {
                throw std::runtime_error("Dijkstra's algorithm cannot handle negative weights");
            }
        }
    }

    std::vector<double> dist(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<bool> visited(vertex_count_, false);
    dist[start] = 0;

    std::priority_queue<std::pair<double, size_t>,
                       std::vector<std::pair<double, size_t>>,
                       std::greater<>> pq;
    pq.push({0, start});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (visited[u]) continue;
        visited[u] = true;

        for (const auto& edge : adjacency_list_[u]) {
            if (!visited[edge.to] && dist[u] + edge.weight < dist[edge.to]) {
                dist[edge.to] = dist[u] + edge.weight;
                pq.push({dist[edge.to], edge.to});
            }
        }
    }

    return dist;
}

std::vector<size_t> Graph::a_star(size_t start, size_t goal,
                                 std::function<double(size_t, size_t)> heuristic) const {
    if (start >= vertex_count_ || goal >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }

    std::vector<double> g_score(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<double> f_score(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<size_t> came_from(vertex_count_, SIZE_MAX);

    g_score[start] = 0;
    f_score[start] = heuristic(start, goal);

    std::priority_queue<std::pair<double, size_t>,
                       std::vector<std::pair<double, size_t>>,
                       std::greater<>> open_set;
    open_set.push({f_score[start], start});

    while (!open_set.empty()) {
        auto p = open_set.top();
        open_set.pop();
        double current_f = p.first;
        size_t current = p.second;

        if (current == goal) {
            std::vector<size_t> path;
            while (current != SIZE_MAX) {
                path.push_back(current);
                current = came_from[current];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& edge : adjacency_list_[current]) {
            double tentative_g_score = g_score[current] + edge.weight;
            if (tentative_g_score < g_score[edge.to]) {
                came_from[edge.to] = current;
                g_score[edge.to] = tentative_g_score;
                f_score[edge.to] = g_score[edge.to] + heuristic(edge.to, goal);
                open_set.push({f_score[edge.to], edge.to});
            }
        }
    }

    return {}; // Путь не найден
}

std::vector<double> Graph::dijkstra_optimized(size_t start) const {
    if (start >= vertex_count_) {
        throw std::out_of_range("Start vertex index out of range");
    }

    std::vector<double> dist(vertex_count_, std::numeric_limits<double>::infinity());
    dist[start] = 0;
    
    std::priority_queue<std::pair<double, size_t>,
                       std::vector<std::pair<double, size_t>>,
                       std::greater<>> pq;
    pq.push({0, start});
    
    std::vector<bool> visited(vertex_count_, false);
    
    while (!pq.empty()) {
        auto p = pq.top();
        pq.pop();
        double d = p.first;
        size_t u = p.second;
        
        if (visited[u]) continue;
        visited[u] = true;
        
        for (const auto& edge : adjacency_list_[u]) {
            if (!visited[edge.to] && dist[u] + edge.weight < dist[edge.to]) {
                dist[edge.to] = dist[u] + edge.weight;
                pq.push({dist[edge.to], edge.to});
            }
        }
    }
    
    return dist;
}

std::vector<size_t> Graph::a_star_optimized(size_t start, size_t goal,
                                          std::function<double(size_t, size_t)> heuristic) const {
    if (start >= vertex_count_ || goal >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }

    std::vector<double> g_score(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<double> f_score(vertex_count_, std::numeric_limits<double>::infinity());
    std::vector<size_t> came_from(vertex_count_, SIZE_MAX);
    std::vector<bool> visited(vertex_count_, false);

    g_score[start] = 0;
    f_score[start] = heuristic(start, goal);

    std::priority_queue<std::pair<double, size_t>,
                       std::vector<std::pair<double, size_t>>,
                       std::greater<>> open_set;
    open_set.push({f_score[start], start});

    while (!open_set.empty()) {
        auto p = open_set.top();
        open_set.pop();
        double current_f = p.first;
        size_t current = p.second;

        if (current == goal) {
            std::vector<size_t> path;
            while (current != SIZE_MAX) {
                path.push_back(current);
                current = came_from[current];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (visited[current]) continue;
        visited[current] = true;

        for (const auto& edge : adjacency_list_[current]) {
            if (!visited[edge.to]) {
                double tentative_g_score = g_score[current] + edge.weight;
                if (tentative_g_score < g_score[edge.to]) {
                    came_from[edge.to] = current;
                    g_score[edge.to] = tentative_g_score;
                    f_score[edge.to] = g_score[edge.to] + heuristic(edge.to, goal);
                    open_set.push({f_score[edge.to], edge.to});
                }
            }
        }
    }

    return {}; // Путь не найден
}

std::vector<std::vector<double>> Graph::floyd_warshall() const {
    std::vector<std::vector<double>> dist(vertex_count_,
        std::vector<double>(vertex_count_, std::numeric_limits<double>::infinity()));

    // Инициализация матрицы расстояний
    for (size_t i = 0; i < vertex_count_; ++i) {
        dist[i][i] = 0;
        for (const auto& edge : adjacency_list_[i]) {
            dist[i][edge.to] = edge.weight;
        }
    }

    // Алгоритм Флойда-Уоршелла
    for (size_t k = 0; k < vertex_count_; ++k) {
        for (size_t i = 0; i < vertex_count_; ++i) {
            for (size_t j = 0; j < vertex_count_; ++j) {
                if (dist[i][k] != std::numeric_limits<double>::infinity() &&
                    dist[k][j] != std::numeric_limits<double>::infinity() &&
                    dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }

    return dist;
}

std::vector<std::vector<double>> Graph::floyd_warshall_parallel(size_t num_threads) const {
    std::vector<std::vector<double>> dist(vertex_count_,
        std::vector<double>(vertex_count_, std::numeric_limits<double>::infinity()));

    // Инициализация матрицы расстояний
    for (size_t i = 0; i < vertex_count_; ++i) {
        dist[i][i] = 0;
        for (const auto& edge : adjacency_list_[i]) {
            dist[i][edge.to] = edge.weight;
        }
    }

    // Алгоритм Флойда-Уоршелла с параллельным выполнением
    for (size_t k = 0; k < vertex_count_; ++k) {
        std::vector<std::thread> threads;
        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                for (size_t i = t; i < vertex_count_; i += num_threads) {
                    for (size_t j = 0; j < vertex_count_; ++j) {
                        if (dist[i][k] != std::numeric_limits<double>::infinity() &&
                            dist[k][j] != std::numeric_limits<double>::infinity() &&
                            dist[i][k] + dist[k][j] < dist[i][j]) {
                            dist[i][j] = dist[i][k] + dist[k][j];
                        }
                    }
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

    return dist;
}

std::vector<std::bitset<64>> Graph::floyd_warshall_bitset() const {
    std::vector<std::bitset<64>> reachable(vertex_count_);

    // Инициализация матрицы достижимости
    for (size_t i = 0; i < vertex_count_; ++i) {
        reachable[i].set(i);
        for (const auto& edge : adjacency_list_[i]) {
            reachable[i].set(edge.to);
        }
    }

    // Алгоритм Флойда-Уоршелла с использованием битовых операций
    for (size_t k = 0; k < vertex_count_; ++k) {
        for (size_t i = 0; i < vertex_count_; ++i) {
            if (reachable[i].test(k)) {
                reachable[i] |= reachable[k];
            }
        }
    }

    return reachable;
}

bool Graph::has_negative_cycle() const {
    if (vertex_count_ == 0) return false;

    std::vector<double> dist(vertex_count_, 0);
    std::vector<size_t> parent(vertex_count_, SIZE_MAX);

    // Запускаем алгоритм Беллмана-Форда
    for (size_t i = 0; i < vertex_count_ - 1; ++i) {
        for (size_t u = 0; u < vertex_count_; ++u) {
            for (const auto& edge : adjacency_list_[u]) {
                if (dist[u] + edge.weight < dist[edge.to]) {
                    dist[edge.to] = dist[u] + edge.weight;
                    parent[edge.to] = u;
                }
            }
        }
    }

    // Проверяем наличие отрицательного цикла
    for (size_t u = 0; u < vertex_count_; ++u) {
        for (const auto& edge : adjacency_list_[u]) {
            if (dist[u] + edge.weight < dist[edge.to]) {
                return true;
            }
        }
    }

    return false;
}

std::vector<size_t> Graph::get_negative_cycle() const {
    if (vertex_count_ == 0) return {};

    std::vector<double> dist(vertex_count_, 0);
    std::vector<size_t> parent(vertex_count_, SIZE_MAX);
    size_t cycle_start = SIZE_MAX;

    // Запускаем алгоритм Беллмана-Форда
    for (size_t i = 0; i < vertex_count_; ++i) {
        cycle_start = SIZE_MAX;
        for (size_t u = 0; u < vertex_count_; ++u) {
            for (const auto& edge : adjacency_list_[u]) {
                if (dist[u] + edge.weight < dist[edge.to]) {
                    dist[edge.to] = dist[u] + edge.weight;
                    parent[edge.to] = u;
                    cycle_start = edge.to;
                }
            }
        }
    }

    if (cycle_start == SIZE_MAX) return {};

    // Находим вершину, принадлежащую циклу
    std::vector<bool> visited(vertex_count_, false);
    size_t v = cycle_start;
    for (size_t i = 0; i < vertex_count_; ++i) {
        v = parent[v];
    }

    // Восстанавливаем цикл
    std::vector<size_t> cycle;
    size_t u = v;
    do {
        cycle.push_back(u);
        u = parent[u];
    } while (u != v);
    cycle.push_back(v);

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
            while (from >= vertex_count_ || to >= vertex_count_) {
                add_vertex();
            }
            add_edge({from, to, weight});
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
        size_t from = edge["from"];
        size_t to = edge["to"];
        double weight = edge["weight"];
        
        while (from >= vertex_count_ || to >= vertex_count_) {
            add_vertex();
        }
        add_edge({from, to, weight});
    }
}

std::string Graph::to_dot() const {
    std::stringstream ss;
    ss << "digraph G {\n";
    
    for (size_t i = 0; i < vertex_count_; ++i) {
        for (const auto& edge : adjacency_list_[i]) {
            ss << "    " << i << " -> " << edge.to << " [label=\"" << edge.weight << "\"];\n";
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
    adjacency_list_.reserve(vertices);
    for (size_t i = 0; i < vertices; ++i) {
        adjacency_list_.push_back({});
        adjacency_list_.back().reserve(edges);
    }
}

routing::GraphStats Graph::get_stats() const {
    update_stats();
    return stats_;
}

void Graph::update_stats() const {
    stats_.max_degree = 0;
    double total_degree = 0;
    for (const auto& edges : adjacency_list_) {
        size_t degree = edges.size();
        stats_.max_degree = std::max<size_t>(stats_.max_degree, degree);
        total_degree += degree;
    }
    stats_.avg_degree = vertex_count_ > 0 ? total_degree / vertex_count_ : 0;
    std::vector<bool> visited(vertex_count_, false);
    std::function<void(size_t)> dfs = [&](size_t v) {
        visited[v] = true;
        for (const auto& edge : adjacency_list_[v]) {
            if (!visited[edge.to]) {
                dfs(edge.to);
            }
        }
    };
    stats_.connected_components = 0;
    for (size_t i = 0; i < vertex_count_; ++i) {
        if (!visited[i]) {
            dfs(i);
            ++stats_.connected_components;
        }
    }
    stats_.is_connected = stats_.connected_components == 1;
}

routing::ProfilingResult Graph::profile_operation(const std::string& name, std::function<void()> operation) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    operation();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    // Оцениваем использование памяти на основе размера графа
    size_t memory_used = vertex_count_ * sizeof(std::vector<Edge>) + 
                        adjacency_list_.size() * sizeof(Edge) +
                        adjacency_map_.size() * sizeof(std::pair<size_t, std::map<size_t, Edge>>);
    
    peak_memory_usage_ = std::max<size_t>(peak_memory_usage_, memory_used);
    total_operations_++;
    
    routing::ProfilingResult result{name, duration, memory_used};
    profiling_results_.push_back(result);
    return result;
}

routing::PerformanceStats Graph::get_performance_stats() const {
    routing::PerformanceStats stats;
    stats.operation_times = profiling_results_;
    stats.total_operations = total_operations_;
    stats.peak_memory_usage = peak_memory_usage_;
    if (!profiling_results_.empty()) {
        size_t total_memory = 0;
        for (const auto& result : profiling_results_) {
            total_memory += result.memory_used;
        }
        stats.average_memory_usage = static_cast<double>(total_memory) / profiling_results_.size();
    }
    return stats;
}

void Graph::reset_performance_stats() {
    profiling_results_.clear();
    peak_memory_usage_ = 0;
    total_operations_ = 0;
}

void Graph::remove_edge(size_t from, size_t to) {
    if (from >= vertex_count_ || to >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    
    // Удаляем из adjacency_list_
    auto& edges = adjacency_list_[from];
    edges.erase(
        std::remove_if(edges.begin(), edges.end(),
            [to](const Edge& edge) { return edge.to == to; }),
        edges.end()
    );
    
    // Удаляем из adjacency_map_
    auto it = adjacency_map_.find(from);
    if (it != adjacency_map_.end()) {
        it->second.erase(to);
        if (it->second.empty()) {
            adjacency_map_.erase(it);
        }
    }
}

void Graph::update_edge_weight(size_t from, size_t to, double weight) {
    if (from >= vertex_count_ || to >= vertex_count_) {
        throw std::out_of_range("Vertex index out of range");
    }
    
    // Обновляем в adjacency_list_
    for (auto& edge : adjacency_list_[from]) {
        if (edge.to == to) {
            edge.weight = weight;
            break;
        }
    }
    
    // Обновляем в adjacency_map_
    auto it = adjacency_map_.find(from);
    if (it != adjacency_map_.end()) {
        auto edge_it = it->second.find(to);
        if (edge_it != it->second.end()) {
            edge_it->second.weight = weight;
        }
    }
}

std::pair<std::vector<double>, bool> Graph::bellman_ford(size_t start) const {
    if (start >= vertex_count_) {
        throw std::out_of_range("Start vertex index out of range");
    }

    std::vector<double> dist(vertex_count_, std::numeric_limits<double>::infinity());
    dist[start] = 0;

    // Релаксация рёбер |V| - 1 раз
    for (size_t i = 0; i < vertex_count_ - 1; ++i) {
        for (size_t u = 0; u < vertex_count_; ++u) {
            for (const auto& edge : adjacency_list_[u]) {
                if (dist[u] != std::numeric_limits<double>::infinity() &&
                    dist[u] + edge.weight < dist[edge.to]) {
                    dist[edge.to] = dist[u] + edge.weight;
                }
            }
        }
    }

    // Проверка на отрицательные циклы
    for (size_t u = 0; u < vertex_count_; ++u) {
        for (const auto& edge : adjacency_list_[u]) {
            if (dist[u] != std::numeric_limits<double>::infinity() &&
                dist[u] + edge.weight < dist[edge.to]) {
                return {dist, true}; // Найден отрицательный цикл
            }
        }
    }

    return {dist, false}; // Отрицательных циклов нет
}

} // namespace routing
