import sys
import os
import time
import psutil
import json
import random

# Добавляем путь к нашей библиотеке
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'src'))

from graph import Graph

def get_memory_usage():
    process = psutil.Process(os.getpid())
    return process.memory_info().rss / (1024 * 1024)  # Конвертируем в МБ

def create_random_graph(n, edges):
    g = Graph(n)
    edge_count = 0
    while edge_count < edges:
        v1 = random.randint(0, n-1)
        v2 = random.randint(0, n-1)
        if v1 != v2 and not g.has_edge(v1, v2):
            g.add_edge(v1, v2, 1.0)
            edge_count += 1
    return g

def test_graph_creation():
    size = 100  # Фиксированный размер графа
    edges = 200  # Фиксированное количество рёбер
    start_mem = get_memory_usage()
    start_time = time.time()
    G = create_random_graph(size, edges)
    end_time = time.time()
    end_mem = get_memory_usage()
    result = {
        "operation": "graph_creation",
        "size": size,
        "time_ms": (end_time - start_time) * 1000,
        "memory_mb": end_mem - start_mem,
        "edges": edges  # Используем точное количество рёбер
    }
    print(json.dumps(result))

def test_dijkstra():
    size = 100  # Фиксированный размер графа
    edges = 200  # Фиксированное количество рёбер
    G = create_random_graph(size, edges)
    start_mem = get_memory_usage()
    start_time = time.time()
    G.dijkstra(0)
    end_time = time.time()
    end_mem = get_memory_usage()
    result = {
        "operation": "dijkstra",
        "size": size,
        "time_ms": (end_time - start_time) * 1000,
        "memory_mb": end_mem - start_mem,
        "edges": edges  # Используем точное количество рёбер
    }
    print(json.dumps(result))

def test_floyd_warshall():
    size = 100  # Фиксированный размер графа
    edges = 200  # Фиксированное количество рёбер
    G = create_random_graph(size, edges)
    start_mem = get_memory_usage()
    start_time = time.time()
    G.floyd_warshall()
    end_time = time.time()
    end_mem = get_memory_usage()
    result = {
        "operation": "floyd_warshall",
        "size": size,
        "time_ms": (end_time - start_time) * 1000,
        "memory_mb": end_mem - start_mem,
        "edges": edges  # Используем точное количество рёбер
    }
    print(json.dumps(result))

def test_floyd_warshall_parallel():
    size = 100  # Фиксированный размер графа
    edges = 200  # Фиксированное количество рёбер
    thread_counts = [2, 4, 8, 16]
    G = create_random_graph(size, edges)
    for threads in thread_counts:
        start_mem = get_memory_usage()
        start_time = time.time()
        G.floyd_warshall_parallel(threads)
        end_time = time.time()
        end_mem = get_memory_usage()
        result = {
            "operation": "floyd_warshall_parallel",
            "size": size,
            "time_ms": (end_time - start_time) * 1000,
            "memory_mb": end_mem - start_mem,
            "edges": edges  # Используем точное количество рёбер
        }
        print(json.dumps(result))

def test_negative_cycle():
    size = 100  # Фиксированный размер графа
    edges = 200  # Фиксированное количество рёбер
    G = create_random_graph(size, edges)
    start_mem = get_memory_usage()
    start_time = time.time()
    G.has_negative_cycle()
    end_time = time.time()
    end_mem = get_memory_usage()
    result = {
        "operation": "negative_cycle",
        "size": size,
        "time_ms": (end_time - start_time) * 1000,
        "memory_mb": end_mem - start_mem,
        "edges": edges  # Используем точное количество рёбер
    }
    print(json.dumps(result))

def test_memory_profile():
    size = 100  # Фиксированный размер графа
    edges = 200  # Фиксированное количество рёбер
    start_mem = get_memory_usage()
    G = create_random_graph(size, edges)
    end_mem = get_memory_usage()
    result = {
        "operation": "memory_profile",
        "size": size,
        "time_ms": 0,
        "memory_mb": end_mem - start_mem,
        "edges": edges  # Используем точное количество рёбер
    }
    print(json.dumps(result))

if __name__ == "__main__":
    test_graph_creation()
    test_dijkstra()
    test_floyd_warshall()
    test_floyd_warshall_parallel()
    test_negative_cycle()
    test_memory_profile() 
