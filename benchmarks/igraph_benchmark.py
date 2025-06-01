import igraph
import time
import psutil
import os
import sys
import json
import random

# Устанавливаем кодировку для вывода
sys.stdout.reconfigure(encoding='utf-8')

def get_memory_usage():
    process = psutil.Process(os.getpid())
    return process.memory_info().rss / (1024 * 1024)  # Конвертируем в МБ

def create_random_graph(n, edges):
    g = igraph.Graph(directed=True)
    g.add_vertices(n)
    edge_set = set()
    while len(edge_set) < edges:
        v1 = random.randint(0, n-1)
        v2 = random.randint(0, n-1)
        if v1 != v2:
            edge_set.add((v1, v2))
    g.add_edges(list(edge_set))
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
            "edges": G.ecount()
        }
        print(json.dumps(result))

def test_dijkstra():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
        G = create_random_graph(size, edges)
        G.es["weight"] = [1.0] * G.ecount()
        start_mem = get_memory_usage()
        start_time = time.time()
        G.distances(source=0, target=size-1, weights="weight")
        end_time = time.time()
        end_mem = get_memory_usage()
        result = {
            "operation": "dijkstra",
            "size": size,
            "time_ms": (end_time - start_time) * 1000,
            "memory_mb": end_mem - start_mem,
            "edges": G.ecount()
        }
        print(json.dumps(result))

def test_floyd_warshall():
    sizes = [100, 200, 500, 1000]
    for size in sizes:
        edges = size * 2
        G = create_random_graph(size, edges)
        G.es["weight"] = [1.0] * G.ecount()
        start_mem = get_memory_usage()
        start_time = time.time()
        G.distances(weights="weight")
        end_time = time.time()
        end_mem = get_memory_usage()
        result = {
            "operation": "floyd_warshall",
            "size": size,
            "time_ms": (end_time - start_time) * 1000,
            "memory_mb": end_mem - start_mem,
            "edges": G.ecount()
        }
        print(json.dumps(result))

if __name__ == "__main__":
    test_graph_creation()
    test_dijkstra()
    test_floyd_warshall()
