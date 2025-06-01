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
    edge_set = set()
    while len(edge_set) < edges:
        v1 = random.randint(0, n-1)
        v2 = random.randint(0, n-1)
        if v1 != v2:
            edge_set.add((v1, v2))
    for v1, v2 in edge_set:
        g.add_edge(v1, v2, 1.0)
    return g

def test_graph_creation():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
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
            "edges": G.edge_count()
        }
        print(json.dumps(result))

def test_dijkstra():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
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
            "edges": G.edge_count()
        }
        print(json.dumps(result))

def test_floyd_warshall():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
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
            "edges": G.edge_count()
        }
        print(json.dumps(result))

if __name__ == "__main__":
    test_graph_creation()
    test_dijkstra()
    test_floyd_warshall() 
