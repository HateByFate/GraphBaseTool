#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif
#include <GraphBaseTool/graph.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <iomanip>
#include <thread>

using namespace routing;
using namespace std::chrono;

// Генерация случайного графа заданного размера
Graph generate_random_graph(size_t vertices, size_t edges_per_vertex, double max_weight = 100.0) {
    Graph graph;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> weight_dist(0.1, max_weight); // Только положительные веса
    
    // Создаем вершины
    for (size_t i = 0; i < vertices; ++i) {
        graph.add_vertex();
    }
    
    // Добавляем рёбра
    for (size_t i = 0; i < vertices; ++i) {
        for (size_t j = 0; j < edges_per_vertex; ++j) {
            size_t to = (i + 1 + j) % vertices; // Обеспечиваем связность
            if (i < vertices && to < vertices) { // Проверка индексов
                double weight = weight_dist(gen);
                graph.add_edge({i, to, weight});
            }
        }
    }
    
    return graph;
}

// Измерение времени выполнения операции
template<typename Func>
double measure_time(Func&& func) {
    auto start = high_resolution_clock::now();
    func();
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count() / 1000.0; // в миллисекундах
}

// Измерение памяти для операции
template<typename Func>
size_t measure_memory(Func&& func) {
#ifdef _WIN32
    HANDLE process = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    DWORD cb = sizeof(pmc);
    
    if (!GetProcessMemoryInfo(process, &pmc, cb)) {
        return 0;
    }
    
    size_t start_memory = pmc.WorkingSetSize;
    func();
    
    if (!GetProcessMemoryInfo(process, &pmc, cb)) {
        return 0;
    }
    
    return pmc.WorkingSetSize > start_memory ? pmc.WorkingSetSize - start_memory : 0;
#else
    func();
    return 0;
#endif
}

void run_performance_tests() {
    std::cout << std::fixed << std::setprecision(2);
    
    // Тест 1: Создание графа
    for (size_t vertices : {100, 200, 500, 1000}) {
        size_t edges = vertices * 2; // Количество рёбер = 2 * количество вершин
        Graph graph;
        double time = measure_time([&]() {
            graph = generate_random_graph(vertices, edges);
        });
        size_t memory = measure_memory([&]() {
            graph = generate_random_graph(vertices, edges);
        });
        double memory_mb = memory / (1024.0 * 1024.0);
        std::cout << "{\"operation\": \"graph_creation\", \"size\": " << vertices
                  << ", \"time_ms\": " << time
                  << ", \"memory_mb\": " << memory_mb
                  << ", \"edges\": " << edges << "}"
                  << std::endl;
    }
    
    // Тест 2: Алгоритм Дейкстры
    for (size_t vertices : {100, 200, 500, 1000}) {
        size_t edges = vertices * 2; // Количество рёбер = 2 * количество вершин
        Graph graph = generate_random_graph(vertices, edges);
        double time = measure_time([&]() {
            graph.dijkstra(0);
        });
        size_t memory = measure_memory([&]() {
            graph.dijkstra(0);
        });
        double memory_mb = memory / (1024.0 * 1024.0);
        std::cout << "{\"operation\": \"dijkstra\", \"size\": " << vertices
                  << ", \"time_ms\": " << time
                  << ", \"memory_mb\": " << memory_mb
                  << ", \"edges\": " << edges << "}"
                  << std::endl;
    }
    
    // Тест 3: Алгоритм Флойда-Уоршелла
    for (size_t vertices : {100, 200, 500, 1000}) {
        size_t edges = vertices * 2; // Количество рёбер = 2 * количество вершин
        Graph graph = generate_random_graph(vertices, edges);
        double time = measure_time([&]() {
            graph.floyd_warshall();
        });
        size_t memory = measure_memory([&]() {
            graph.floyd_warshall();
        });
        double memory_mb = memory / (1024.0 * 1024.0);
        std::cout << "{\"operation\": \"floyd_warshall\", \"size\": " << vertices
                  << ", \"time_ms\": " << time
                  << ", \"memory_mb\": " << memory_mb
                  << ", \"edges\": " << edges << "}"
                  << std::endl;
    }
    
    // Тест 4: Параллельный алгоритм Флойда-Уоршелла
    for (size_t vertices : {100, 200, 500, 1000}) {
        size_t edges = vertices * 2; // Количество рёбер = 2 * количество вершин
        Graph graph = generate_random_graph(vertices, edges);
        for (size_t threads : {1, 2, 4, 8}) {
            double time = measure_time([&]() {
                graph.floyd_warshall_parallel(threads);
            });
            size_t memory = measure_memory([&]() {
                graph.floyd_warshall_parallel(threads);
            });
            double memory_mb = memory / (1024.0 * 1024.0);
            std::cout << "{\"operation\": \"floyd_warshall_parallel\", \"size\": " << vertices
                      << ", \"time_ms\": " << time
                      << ", \"memory_mb\": " << memory_mb
                      << ", \"edges\": " << edges
                      << ", \"threads\": " << threads << "}"
                      << std::endl;
        }
    }
    
    // Тест 5: Поиск отрицательных циклов
    for (size_t vertices : {100, 200, 500, 1000}) {
        size_t edges = vertices * 2; // Количество рёбер = 2 * количество вершин
        Graph graph = generate_random_graph(vertices, edges);
        double time = measure_time([&]() {
            graph.has_negative_cycle();
        });
        size_t memory = measure_memory([&]() {
            graph.has_negative_cycle();
        });
        double memory_mb = memory / (1024.0 * 1024.0);
        std::cout << "{\"operation\": \"negative_cycle\", \"size\": " << vertices
                  << ", \"time_ms\": " << time
                  << ", \"memory_mb\": " << memory_mb
                  << ", \"edges\": " << edges << "}"
                  << std::endl;
    }
    
    // Тест 6: Профилирование памяти
    for (size_t vertices : {100, 200, 500, 1000}) {
        size_t edges = vertices * 2; // Количество рёбер = 2 * количество вершин
        Graph graph;
        size_t memory = measure_memory([&]() {
            graph = generate_random_graph(vertices, edges);
        });
        double memory_mb = memory / (1024.0 * 1024.0);
        std::cout << "{\"operation\": \"memory_profile\", \"size\": " << vertices
                  << ", \"memory_mb\": " << memory_mb
                  << ", \"edges\": " << edges << "}"
                  << std::endl;
    }
}

int main() {
    try {
        run_performance_tests();
    } catch (const std::exception& ex) {
        std::cerr << "{\"error\": \"" << ex.what() << "\"}" << std::endl;
    } catch (...) {
        std::cerr << "{\"error\": \"Неизвестное исключение!\"}" << std::endl;
    }
    return 0;
} 
