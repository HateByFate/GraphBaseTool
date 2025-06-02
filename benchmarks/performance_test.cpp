#include <GraphBaseTool/graph.hpp>
#include <iostream>
#include <chrono>
#include <random>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>

using namespace routing;
using json = nlohmann::json;

// Функция для генерации случайного графа
Graph generateRandomGraph(size_t numVertices, size_t numEdges) {
    Graph graph;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> weightDist(1.0, 100.0);
    
    // Добавляем вершины
    for (size_t i = 0; i < numVertices; ++i) {
        graph.add_vertex();
    }
    
    // Создаем все возможные рёбра
    std::vector<std::pair<size_t, size_t>> possible_edges;
    for (size_t i = 0; i < numVertices; ++i) {
        for (size_t j = 0; j < numVertices; ++j) {
            if (i != j) {
                possible_edges.emplace_back(i, j);
            }
        }
    }
    
    // Перемешиваем рёбра
    std::shuffle(possible_edges.begin(), possible_edges.end(), gen);
    
    // Добавляем нужное количество рёбер
    for (size_t i = 0; i < numEdges && i < possible_edges.size(); ++i) {
        const auto& [from, to] = possible_edges[i];
        double weight = weightDist(gen);
        graph.add_edge({from, to, weight});
    }
    
    return graph;
}

// Функция для тестирования алгоритма Дейкстры
json testDijkstra(const Graph& graph, size_t startVertex) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = graph.dijkstra_optimized(startVertex);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end - start;
    auto stats = graph.get_performance_stats();
    
    return {
        {"operation", "dijkstra"},
        {"size", graph.vertex_count()},
        {"edges", graph.edge_count()},
        {"time_ms", duration.count()},
        {"memory_mb", stats.peak_memory_usage / (1024.0 * 1024.0)}
    };
}

// Функция для тестирования алгоритма A*
json testAStar(const Graph& graph, size_t startVertex, size_t endVertex) {
    auto heuristic = [](size_t from, size_t to) {
        return 0.0; // Простая эвристика для теста
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = graph.a_star_optimized(startVertex, endVertex, heuristic);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end - start;
    auto stats = graph.get_performance_stats();
    
    return {
        {"operation", "a_star"},
        {"size", graph.vertex_count()},
        {"edges", graph.edge_count()},
        {"time_ms", duration.count()},
        {"memory_mb", stats.peak_memory_usage / (1024.0 * 1024.0)}
    };
}

// Функция для тестирования алгоритма Флойда-Уоршелла
json testFloydWarshall(const Graph& graph) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = graph.floyd_warshall_parallel(4); // Используем 4 потока
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end - start;
    auto stats = graph.get_performance_stats();
    
    return {
        {"operation", "floyd_warshall_parallel"},
        {"size", graph.vertex_count()},
        {"edges", graph.edge_count()},
        {"time_ms", duration.count()},
        {"memory_mb", stats.peak_memory_usage / (1024.0 * 1024.0)}
    };
}

int main() {
    // Размеры графов для тестирования
    std::vector<size_t> sizes = {100, 500, 1000, 2000};
    
    for (size_t size : sizes) {
        size_t edges = size * 2; // Фиксированное соотношение: количество рёбер = 2 * количество вершин
        Graph testGraph = generateRandomGraph(size, edges);
        
        // Запускаем тесты и выводим результаты в JSON
        std::cout << testDijkstra(testGraph, 0).dump() << std::endl;
        std::cout << testAStar(testGraph, 0, size - 1).dump() << std::endl;
        std::cout << testFloydWarshall(testGraph).dump() << std::endl;
    }
    
    return 0;
} 
