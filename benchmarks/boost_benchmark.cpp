#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <windows.h>
#include <psapi.h>
#include <fcntl.h>
#include <io.h>
#include <algorithm>
#include <limits>
#include <functional>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/property_map.hpp>
#include <atomic>
#include <thread>

using namespace std;
using namespace boost;
using std::cout;
using std::endl;
using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, boost::property<boost::edge_weight_t, double>>;

// Функция для настройки консоли
void setup_console() {
    // Устанавливаем кодировку UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    // Устанавливаем шрифт консоли
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
}

// Функция для измерения памяти
double get_memory_usage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        // Используем только PrivateUsage, так как WorkingSetSize может быть неточным
        return pmc.PrivateUsage / (1024.0 * 1024.0);
    }
    return 0.0;
}

// Функция для измерения базового использования памяти
double get_base_memory_usage() {
    static double base_memory = -1.0;
    if (base_memory < 0) {
        // Измеряем базовое использование памяти несколько раз
        std::vector<double> measurements;
        for (int i = 0; i < 5; ++i) {
            measurements.push_back(get_memory_usage());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // Используем минимальное значение как базовое
        base_memory = *std::min_element(measurements.begin(), measurements.end());
    }
    return base_memory;
}

// Функция для создания случайного графа
template<typename Graph>
void create_random_graph(Graph& g, int num_vertices, int num_edges) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> vertex_dist(0, num_vertices - 1);
    std::uniform_real_distribution<> weight_dist(1.0, 100.0);

    // Добавляем вершины
    for (int i = 0; i < num_vertices; ++i) {
        add_vertex(g);
    }

    // Добавляем рёбра
    for (int i = 0; i < num_edges; ++i) {
        int v1 = vertex_dist(gen);
        int v2 = vertex_dist(gen);
        if (v1 != v2) {
            add_edge(v1, v2, weight_dist(gen), g);
        }
    }
}

// Функция для измерения памяти с усреднением
double measure_memory_usage(const std::function<void()>& func) {
    std::vector<double> measurements;
    double base_memory = get_base_memory_usage();
    
    for (int i = 0; i < 3; ++i) {  // Делаем 3 измерения
        // Очищаем память перед измерением
        std::vector<int> dummy;
        dummy.reserve(1000000);  // Выделяем и освобождаем память
        dummy.clear();
        
        double start_memory = get_memory_usage();
        
        // Создаем отдельный поток для мониторинга памяти
        std::atomic<double> peak_memory{0.0};
        std::atomic<bool> should_stop{false};
        
        std::thread monitor_thread([&]() {
            while (!should_stop) {
                double current_memory = get_memory_usage();
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
        
        // Вычисляем разницу с учетом базового использования памяти
        double memory_diff = peak_memory.load() - start_memory;
        if (memory_diff < 0) {
            memory_diff = 0.0;  // Предотвращаем отрицательные значения
        }
        measurements.push_back(memory_diff);
    }
    
    // Сортируем и возвращаем медианное значение
    std::sort(measurements.begin(), measurements.end());
    return measurements[1];
}

// Функция для измерения времени с усреднением
double measure_time(const std::function<void()>& func, int warmup_iterations = 3, int measurement_iterations = 5) {
    // Разогрев
    for (int i = 0; i < warmup_iterations; ++i) {
        func();
    }
    
    // Измерения
    std::vector<double> measurements;
    for (int i = 0; i < measurement_iterations; ++i) {
        auto start = std::chrono::steady_clock::now();
        func();
        auto end = std::chrono::steady_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        measurements.push_back(time_ms);
    }
    
    // Сортируем и возвращаем медианное значение
    std::sort(measurements.begin(), measurements.end());
    return measurements[measurements.size() / 2];
}

// Тест создания графа
void test_graph_creation(int num_vertices, int num_edges) {
    Graph g;
    double time_ms = measure_time([&]() {
        create_random_graph(g, num_vertices, num_edges);
    });
    double memory_usage = measure_memory_usage([&]() {
        create_random_graph(g, num_vertices, num_edges);
    });
    
    cout << "{\"operation\": \"graph_creation\", \"size\": " << num_vertices
         << ", \"time_ms\": " << time_ms
         << ", \"memory_mb\": " << memory_usage
         << ", \"edges\": " << num_edges << "}" << endl;
}

// Тест алгоритма Дейкстры
void test_dijkstra(int num_vertices, int num_edges) {
    Graph g;
    create_random_graph(g, num_vertices, num_edges);
    std::vector<double> distances(num_vertices);
    std::vector<graph_traits<Graph>::vertex_descriptor> predecessors(num_vertices);
    
    double time_ms = measure_time([&]() {
        dijkstra_shortest_paths(g, 0,
            predecessor_map(make_iterator_property_map(predecessors.begin(), get(vertex_index, g)))
            .distance_map(make_iterator_property_map(distances.begin(), get(vertex_index, g))));
    });
    double memory_usage = measure_memory_usage([&]() {
        dijkstra_shortest_paths(g, 0,
            predecessor_map(make_iterator_property_map(predecessors.begin(), get(vertex_index, g)))
            .distance_map(make_iterator_property_map(distances.begin(), get(vertex_index, g))));
    });
    
    cout << "{\"operation\": \"dijkstra\", \"size\": " << num_vertices
         << ", \"time_ms\": " << time_ms
         << ", \"memory_mb\": " << memory_usage
         << ", \"edges\": " << num_edges << "}" << endl;
}

// Тест алгоритма Флойда-Уоршелла
void test_floyd_warshall(int num_vertices, int num_edges) {
    Graph g;
    create_random_graph(g, num_vertices, num_edges);
    std::vector<std::vector<double>> distances(num_vertices, std::vector<double>(num_vertices, std::numeric_limits<double>::infinity()));
    graph_traits<Graph>::edge_iterator ei, ei_end;
    for (std::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
        int u = source(*ei, g);
        int v = target(*ei, g);
        distances[u][v] = get(edge_weight, g, *ei);
    }
    for (int i = 0; i < num_vertices; ++i) {
        distances[i][i] = 0;
    }
    
    double time_ms = measure_time([&]() {
        floyd_warshall_all_pairs_shortest_paths(g, distances);
    });
    double memory_usage = measure_memory_usage([&]() {
        floyd_warshall_all_pairs_shortest_paths(g, distances);
    });
    
    cout << "{\"operation\": \"floyd_warshall\", \"size\": " << num_vertices
         << ", \"time_ms\": " << time_ms
         << ", \"memory_mb\": " << memory_usage
         << ", \"edges\": " << num_edges << "}" << endl;
}

int main() {
    setup_console();
    // Тестируем разные размеры графов
    std::vector<std::pair<int, int>> test_cases = {
        {100, 1000},    // 100 вершин, 1000 рёбер
        {200, 4000},    // 200 вершин, 4000 рёбер
        {500, 25000},   // 500 вершин, 25000 рёбер
        {1000, 100000}  // 1000 вершин, 100000 рёбер
    };
    
    for (const auto& [vertices, edges] : test_cases) {
        test_graph_creation(vertices, edges);
        test_dijkstra(vertices, edges);
        test_floyd_warshall(vertices, edges);
    }
    return 0;
} 
