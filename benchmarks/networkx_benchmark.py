import networkx as nx
import time
import psutil
import os
import sys
import json
import gc
import random
import threading
import queue

# Устанавливаем кодировку для вывода
if sys.stdout.encoding != 'utf-8':
    sys.stdout.reconfigure(encoding='utf-8')

def get_memory_usage():
    process = psutil.Process()
    return process.memory_info().rss / (1024 * 1024)  # Конвертируем в МБ

def measure_peak_memory(func, *args, **kwargs):
    process = psutil.Process()
    peak_memory = 0
    result = None
    
    def memory_monitor():
        nonlocal peak_memory
        while not stop_event.is_set():
            current_memory = process.memory_info().rss / (1024 * 1024)
            peak_memory = max(peak_memory, current_memory)
            time.sleep(0.001)  # Проверяем каждую миллисекунду
    
    stop_event = threading.Event()
    monitor_thread = threading.Thread(target=memory_monitor)
    monitor_thread.start()
    
    try:
        result = func(*args, **kwargs)
    finally:
        stop_event.set()
        monitor_thread.join()
    
    return peak_memory, result

def measure_memory_usage(func, *args, **kwargs):
    # Выполняем несколько измерений для усреднения
    measurements = []
    for _ in range(3):  # Делаем 3 измерения
        gc.collect()  # Очищаем память перед измерением
        start_mem = get_memory_usage()
        peak_mem, _ = measure_peak_memory(func, *args, **kwargs)
        gc.collect()  # Очищаем память после выполнения
        measurements.append(peak_mem - start_mem)
    
    # Возвращаем среднее значение, исключая выбросы
    measurements.sort()
    return measurements[1]  # Возвращаем медианное значение

def create_random_graph(n, edges):
    G = nx.gnm_random_graph(n, edges, directed=True)
    # Убеждаемся, что граф связный
    while not nx.is_strongly_connected(G):
        # Добавляем случайное ребро
        u = random.randint(0, n-1)
        v = random.randint(0, n-1)
        if u != v and not G.has_edge(u, v):
            G.add_edge(u, v)
    return G

def measure_time(func, *args, warmup_iterations=3, measurement_iterations=5, **kwargs):
    # Разогрев
    for _ in range(warmup_iterations):
        func(*args, **kwargs)
    
    # Измерения
    measurements = []
    for _ in range(measurement_iterations):
        start_time = time.perf_counter()
        func(*args, **kwargs)
        end_time = time.perf_counter()
        time_ms = (end_time - start_time) * 1000
        measurements.append(time_ms)
    
    # Сортируем и возвращаем медианное значение
    measurements.sort()
    return measurements[len(measurements) // 2]

def test_graph_creation():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
        time_ms = measure_time(create_random_graph, size, edges)
        memory_usage = measure_memory_usage(create_random_graph, size, edges)
        G = create_random_graph(size, edges)
        result = {
            "operation": "graph_creation",
            "size": size,
            "time_ms": time_ms,
            "memory_mb": memory_usage,
            "edges": G.number_of_edges()
        }
        print(json.dumps(result))

def test_dijkstra():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
        G = create_random_graph(size, edges)
        for (u, v) in G.edges():
            G[u][v]['weight'] = 1.0
        time_ms = measure_time(lambda: nx.single_source_dijkstra_path_length(G, 0))
        memory_usage = measure_memory_usage(lambda: nx.single_source_dijkstra_path_length(G, 0))
        result = {
            "operation": "dijkstra",
            "size": size,
            "time_ms": time_ms,
            "memory_mb": memory_usage,
            "edges": G.number_of_edges()
        }
        print(json.dumps(result))

def test_floyd_warshall():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
        G = create_random_graph(size, edges)
        for (u, v) in G.edges():
            G[u][v]['weight'] = 1.0
        time_ms = measure_time(lambda: nx.floyd_warshall_numpy(G))
        memory_usage = measure_memory_usage(lambda: nx.floyd_warshall_numpy(G))
        result = {
            "operation": "floyd_warshall",
            "size": size,
            "time_ms": time_ms,
            "memory_mb": memory_usage,
            "edges": G.number_of_edges()
        }
        print(json.dumps(result))

def test_astar():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
        G = create_random_graph(size, edges)
        for (u, v) in G.edges():
            G[u][v]['weight'] = 1.0
        # Простая эвристика - евклидово расстояние
        def heuristic(u, v):
            return 0.0  # Простая эвристика для теста
        time_ms = measure_time(lambda: nx.astar_path(G, 0, size-1, heuristic))
        memory_usage = measure_memory_usage(lambda: nx.astar_path(G, 0, size-1, heuristic))
        result = {
            "operation": "a_star",
            "size": size,
            "time_ms": time_ms,
            "memory_mb": memory_usage,
            "edges": G.number_of_edges()
        }
        print(json.dumps(result))

if __name__ == "__main__":
    test_graph_creation()
    test_dijkstra()
    test_floyd_warshall()
    test_astar() 
