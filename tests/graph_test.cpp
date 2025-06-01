#include <GraphBaseTool/graph.hpp>
#include "test_framework.hpp"
#include <vector>
#include <limits>

TEST(graph_creation) {
    routing::Graph graph;
    ASSERT(graph.vertex_count() == 0);
}

TEST(add_vertex) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    ASSERT(graph.vertex_count() == 1);
    ASSERT(v1 == 0);
}

TEST(add_multiple_vertices) {
    routing::Graph graph;
    std::vector<size_t> vertices;
    for (int i = 0; i < 10; ++i) {
        vertices.push_back(graph.add_vertex());
    }
    ASSERT(graph.vertex_count() == 10);
    for (size_t i = 0; i < vertices.size(); ++i) {
        ASSERT(vertices[i] == i);
    }
}

TEST(add_edge) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    routing::Edge edge{v1, v2, 10};
    graph.add_edge(edge);
    ASSERT(graph.get_edge_weight(v1, v2) == 10);
}

TEST(add_multiple_edges) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, 10});
    graph.add_edge({v2, v3, 20});
    graph.add_edge({v1, v3, 30});
    
    ASSERT(graph.get_edge_weight(v1, v2) == 10);
    ASSERT(graph.get_edge_weight(v2, v3) == 20);
    ASSERT(graph.get_edge_weight(v1, v3) == 30);
}

TEST(remove_edge) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    graph.add_edge({v1, v2, 10});
    graph.remove_edge(v1, v2);
    ASSERT(graph.get_edge_weight(v1, v2) == std::numeric_limits<double>::infinity());
}

TEST(update_edge_weight) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    graph.add_edge({v1, v2, 10});
    graph.update_edge_weight(v1, v2, 20);
    ASSERT(graph.get_edge_weight(v1, v2) == 20);
}

TEST(dijkstra_shortest_path) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    routing::Edge edge1{v1, v2, 10};
    routing::Edge edge2{v2, v3, 20};
    graph.add_edge(edge1);
    graph.add_edge(edge2);
    
    auto distances = graph.dijkstra(v1);
    ASSERT(distances[v2] == 10);
    ASSERT(distances[v3] == 30);
}

TEST(dijkstra_disconnected_graph) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, 10});
    
    auto distances = graph.dijkstra(v1);
    ASSERT(distances[v2] == 10);
    ASSERT(distances[v3] == std::numeric_limits<double>::infinity());
}

TEST(dijkstra_negative_weights) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, -10});
    graph.add_edge({v2, v3, 20});
    
    auto distances = graph.dijkstra(v1);
    ASSERT(distances[v2] == -10);
    ASSERT(distances[v3] == 10);
}

TEST(negative_cycle_detection) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    routing::Edge edge1{v1, v2, 10};
    routing::Edge edge2{v2, v3, -20};
    routing::Edge edge3{v3, v1, -5};
    graph.add_edge(edge1);
    graph.add_edge(edge2);
    graph.add_edge(edge3);
    
    ASSERT(graph.has_negative_cycle());
    auto cycle = graph.get_negative_cycle();
    ASSERT(!cycle.empty());
}

TEST(no_negative_cycle) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, 10});
    graph.add_edge({v2, v3, 20});
    graph.add_edge({v3, v1, 30});
    
    ASSERT(!graph.has_negative_cycle());
    auto cycle = graph.get_negative_cycle();
    ASSERT(cycle.empty());
}

TEST(bellman_ford_shortest_path) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, 10});
    graph.add_edge({v2, v3, 20});
    
    auto result = graph.bellman_ford(v1);
    ASSERT(!result.second);
    ASSERT(result.first[v2] == 10);
    ASSERT(result.first[v3] == 30);
}

TEST(bellman_ford_negative_cycle) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, 10});
    graph.add_edge({v2, v3, -20});
    graph.add_edge({v3, v1, -5});
    
    auto result = graph.bellman_ford(v1);
    ASSERT(result.second);
}

TEST(floyd_warshall) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, 10});
    graph.add_edge({v2, v3, 20});
    graph.add_edge({v1, v3, 30});
    
    auto distances = graph.floyd_warshall();
    ASSERT(distances[v1][v2] == 10);
    ASSERT(distances[v1][v3] == 30);
    ASSERT(distances[v2][v3] == 20);
}

TEST(floyd_warshall_negative_weights) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, -10});
    graph.add_edge({v2, v3, 20});
    graph.add_edge({v1, v3, 30});
    
    auto distances = graph.floyd_warshall();
    ASSERT(distances[v1][v2] == -10);
    ASSERT(distances[v1][v3] == 10);
    ASSERT(distances[v2][v3] == 20);
}

TEST(floyd_warshall_negative_cycle) {
    routing::Graph graph;
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    size_t v3 = graph.add_vertex();
    
    graph.add_edge({v1, v2, 10});
    graph.add_edge({v2, v3, -20});
    graph.add_edge({v3, v1, -5});
    
    auto distances = graph.floyd_warshall();
    ASSERT(distances[v1][v1] < 0); // Отрицательный цикл
    ASSERT(distances[v2][v2] < 0);
    ASSERT(distances[v3][v3] < 0);
}

int main() {
    return TestFramework::getInstance().runTests() ? 0 : 1;
} 
