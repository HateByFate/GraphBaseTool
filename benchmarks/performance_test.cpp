#include <GraphBaseTool/graph.hpp>
#include <iostream>
#include <chrono>
#include <random>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <atomic>
#include <thread>

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
json testDijkstra(Graph& graph, size_t startVertex) {
    graph.reset_performance_stats();
    auto result = graph.profile_operation("dijkstra", [&]() -> std::vector<double> {
        return graph.dijkstra(startVertex);
    });
    return {
        {"operation", "dijkstra"},
        {"size", graph.vertex_count()},
        {"edges", graph.edge_count()},
        {"time_ms", std::chrono::duration<double, std::milli>(result.duration).count()},
        {"memory_mb", result.memory_used / (1024.0 * 1024.0)}
    };
}

// Функция для тестирования алгоритма A*
json testAStar(Graph& graph, size_t startVertex, size_t endVertex) {
    graph.reset_performance_stats();
    auto heuristic = [](size_t from, size_t to) {
        return 0.0; // Простая эвристика для теста
    };
    auto result = graph.profile_operation("a_star", [&]() -> std::vector<size_t> {
        return graph.a_star(startVertex, endVertex, heuristic);
    });
    return {
        {"operation", "a_star"},
        {"size", graph.vertex_count()},
        {"edges", graph.edge_count()},
        {"time_ms", std::chrono::duration<double, std::milli>(result.duration).count()},
        {"memory_mb", result.memory_used / (1024.0 * 1024.0)}
    };
}

// Функция для тестирования алгоритма Флойда-Уоршелла
json testFloydWarshall(Graph& graph) {
    graph.reset_performance_stats();
    auto result = graph.profile_operation("floyd_warshall", [&]() -> std::vector<std::vector<double>> {
        return graph.floyd_warshall();
    });
    
    auto stats = graph.get_performance_stats();
    return {
        {"operation", "floyd_warshall"},
        {"size", graph.vertex_count()},
        {"edges", graph.edge_count()},
        {"time_ms", std::chrono::duration<double, std::milli>(result.duration).count()},
        {"memory_mb", static_cast<double>(result.memory_used) / (1024.0 * 1024.0)}
    };
}

// Функция для тестирования создания графа
json testGraphCreation(size_t numVertices, size_t numEdges) {
    Graph graph;
    auto result = graph.profile_operation("graph_creation", [&]() -> void {
        graph = generateRandomGraph(numVertices, numEdges);
    });
    return {
        {"operation", "graph_creation"},
        {"size", numVertices},
        {"edges", numEdges},
        {"time_ms", std::chrono::duration<double, std::milli>(result.duration).count()},
        {"memory_mb", result.memory_used / (1024.0 * 1024.0)}
    };
}

// Функция для тестирования поиска отрицательных циклов
json testNegativeCycle(Graph& graph) {
    graph.reset_performance_stats();
    auto result = graph.profile_operation("negative_cycle", [&]() -> bool {
        return graph.has_negative_cycle();
    });
    
    auto stats = graph.get_performance_stats();
    return {
        {"operation", "negative_cycle"},
        {"size", graph.vertex_count()},
        {"edges", graph.edge_count()},
        {"time_ms", std::chrono::duration<double, std::milli>(result.duration).count()},
        {"memory_mb", static_cast<double>(result.memory_used) / (1024.0 * 1024.0)}
    };
}

// Функция для тестирования профилирования памяти
json testMemoryProfile(Graph& graph) {
    graph.reset_performance_stats();
    auto result = graph.profile_operation("memory_profile", [&]() -> void {
        // Пустая операция для измерения базового использования памяти
    });
    
    auto stats = graph.get_performance_stats();
    return {
        {"operation", "memory_profile"},
        {"size", graph.vertex_count()},
        {"edges", graph.edge_count()},
        {"time_ms", std::chrono::duration<double, std::milli>(result.duration).count()},
        {"memory_mb", static_cast<double>(stats.peak_memory_usage) / (1024.0 * 1024.0)}
    };
}

template<typename Func>
double measure_time(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

template<typename Func>
double measure_memory_usage(Func&& func) {
    std::vector<double> measurements;
    for (int i = 0; i < 3; ++i) {  // Делаем 3 измерения
        PROCESS_MEMORY_COUNTERS_EX pmc;
        DWORD cb = sizeof(pmc);
        
        if (!GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, cb)) {
            return 0;
        }
        
        double start_memory = (pmc.PrivateUsage + pmc.WorkingSetSize) / (1024.0 * 1024.0);
        
        // Создаем отдельный поток для мониторинга памяти
        std::atomic<double> peak_memory{0.0};
        std::atomic<bool> should_stop{false};
        
        std::thread monitor_thread([&]() {
            while (!should_stop) {
                if (!GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, cb)) {
                    continue;
                }
                double current_memory = (pmc.PrivateUsage + pmc.WorkingSetSize) / (1024.0 * 1024.0);
                double current_peak = peak_memory.load();
                while (current_memory > current_peak) {
                    if (peak_memory.compare_exchange_weak(current_peak, current_memory)) {
                        break;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        // Выполняем функцию
        func();
        
        // Останавливаем мониторинг
        should_stop = true;
        monitor_thread.join();
        
        measurements.push_back(peak_memory.load() - start_memory);
    }
    
    // Сортируем и возвращаем медианное значение
    std::sort(measurements.begin(), measurements.end());
    return measurements[1];
}

void run_performance_tests() {
    std::cout << std::fixed << std::setprecision(2);
    
    // Тест 1: Создание графа
    for (size_t vertices : {100, 200, 500, 1000}) {
        size_t edges = vertices * 2; // Количество рёбер = 2 * количество вершин
        Graph graph;
        double time = measure_time([&]() {
            graph = generateRandomGraph(vertices, edges);
        });
        double memory = measure_memory_usage([&]() {
            graph = generateRandomGraph(vertices, edges);
        });
        std::cout << "{\"operation\": \"graph_creation\", \"size\": " << vertices
                  << ", \"time_ms\": " << time
                  << ", \"memory_mb\": " << memory
                  << ", \"edges\": " << edges << "}"
                  << std::endl;
    }
    
    // ... rest of the code ...
}

int main() {
    // Размеры графов для тестирования
    std::vector<size_t> sizes = {100, 200, 500, 1000};
    
    for (size_t size : sizes) {
        size_t edges = size * 2; // Фиксированное соотношение: количество рёбер = 2 * количество вершин
        
        // Тест создания графа
        auto creation_result = testGraphCreation(size, edges);
        std::cout << json({
            {"library", "GraphBaseTool"},
            {"operation", "graph_creation"},
            {"size", size},
            {"edges", edges},
            {"time_ms", creation_result["time_ms"]},
            {"memory_mb", creation_result["memory_mb"]}
        }).dump() << std::endl;
        
        // Создаем граф для остальных тестов
        Graph testGraph = generateRandomGraph(size, edges);
        
        // Сброс статистики перед каждым тестом
        testGraph.reset_performance_stats();
        auto dijkstra_result = testDijkstra(testGraph, 0);
        std::cout << json({
            {"library", "GraphBaseTool"},
            {"operation", "dijkstra"},
            {"size", size},
            {"edges", edges},
            {"time_ms", dijkstra_result["time_ms"]},
            {"memory_mb", dijkstra_result["memory_mb"]}
        }).dump() << std::endl;
        
        testGraph.reset_performance_stats();
        auto astar_result = testAStar(testGraph, 0, size - 1);
        std::cout << json({
            {"library", "GraphBaseTool"},
            {"operation", "a_star"},
            {"size", size},
            {"edges", edges},
            {"time_ms", astar_result["time_ms"]},
            {"memory_mb", astar_result["memory_mb"]}
        }).dump() << std::endl;
        
        testGraph.reset_performance_stats();
        auto floyd_result = testFloydWarshall(testGraph);
        std::cout << json({
            {"library", "GraphBaseTool"},
            {"operation", "floyd_warshall"},
            {"size", size},
            {"edges", edges},
            {"time_ms", floyd_result["time_ms"]},
            {"memory_mb", floyd_result["memory_mb"]}
        }).dump() << std::endl;
        
        testGraph.reset_performance_stats();
        auto result_parallel_profile = testGraph.profile_operation("floyd_warshall_parallel", [&]() {
            testGraph.floyd_warshall_parallel(4);
        });
        std::cout << json({
            {"library", "GraphBaseTool"},
            {"operation", "floyd_warshall_parallel"},
            {"size", size},
            {"edges", edges},
            {"time_ms", std::chrono::duration<double, std::milli>(result_parallel_profile.duration).count()},
            {"memory_mb", static_cast<double>(result_parallel_profile.memory_used) / (1024.0 * 1024.0)}
        }).dump() << std::endl;
        
        testGraph.reset_performance_stats();
        auto negative_cycle_result = testNegativeCycle(testGraph);
        std::cout << json({
            {"library", "GraphBaseTool"},
            {"operation", "negative_cycle"},
            {"size", size},
            {"edges", edges},
            {"time_ms", negative_cycle_result["time_ms"]},
            {"memory_mb", negative_cycle_result["memory_mb"]}
        }).dump() << std::endl;
        
        testGraph.reset_performance_stats();
        auto memory_profile_result = testMemoryProfile(testGraph);
        std::cout << json({
            {"library", "GraphBaseTool"},
            {"operation", "memory_profile"},
            {"size", size},
            {"edges", edges},
            {"time_ms", memory_profile_result["time_ms"]},
            {"memory_mb", memory_profile_result["memory_mb"]}
        }).dump() << std::endl;
    }
    
    return 0;
} 
