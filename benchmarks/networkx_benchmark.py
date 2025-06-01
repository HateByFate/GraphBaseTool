import networkx as nx
import time
import psutil
import os
import sys
import json

# Устанавливаем кодировку для вывода
sys.stdout.reconfigure(encoding='utf-8')

def get_memory_usage():
    process = psutil.Process(os.getpid())
    return process.memory_info().rss / (1024 * 1024)  # Конвертируем в МБ

def create_random_graph(n, edges):
    G = nx.gnm_random_graph(n, edges, directed=True)
    return G

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
        start_mem = get_memory_usage()
        start_time = time.time()
        nx.single_source_dijkstra_path_length(G, 0)
        end_time = time.time()
        end_mem = get_memory_usage()
        result = {
            "operation": "dijkstra",
            "size": size,
            "time_ms": (end_time - start_time) * 1000,
            "memory_mb": end_mem - start_mem,
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
        start_mem = get_memory_usage()
        start_time = time.time()
        nx.floyd_warshall_numpy(G)
        end_time = time.time()
        end_mem = get_memory_usage()
        result = {
            "operation": "floyd_warshall",
            "size": size,
            "time_ms": (end_time - start_time) * 1000,
            "memory_mb": end_mem - start_mem,
            "edges": G.number_of_edges()
        }
        print(json.dumps(result))

if __name__ == "__main__":
    test_graph_creation()
    test_dijkstra()
    test_floyd_warshall() 
