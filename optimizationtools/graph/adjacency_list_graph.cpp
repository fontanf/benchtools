#include "optimizationtools/graph/adjacency_list_graph.hpp"

#include "optimizationtools/containers/indexed_set.hpp"

#include <cstdint>
#include <vector>
#include <fstream>

using namespace optimizationtools;

AdjacencyListGraph::AdjacencyListGraph(
        std::string instance_path,
        std::string format)
{
    std::ifstream file(instance_path);
    if (!file.good())
        throw std::runtime_error(
                "Unable to open file \"" + instance_path + "\".");

    if (format == "dimacs" || format == "dimacs1992") {
        read_dimacs1992(file);
    } else if (format == "dimacs2010") {
        read_dimacs2010(file);
    } else if (format == "matrixmarket") {
        read_matrixmarket(file);
    } else if (format == "snap") {
        read_snap(file);
    } else if (format == "chaco") {
        read_chaco(file);
    } else {
        throw std::invalid_argument(
                "Unknown instance format \"" + format + "\".");
    }
}

VertexId AdjacencyListGraph::add_vertex(Weight weight)
{
    VertexId vertex_id = vertices_.size();

    Vertex vertex;
    vertex.weight = weight;
    vertices_.push_back(vertex);

    total_weight_ += weight;
    return vertex_id;
}

void AdjacencyListGraph::set_weight(
        VertexId vertex_id,
        Weight weight)
{
    total_weight_ -= vertices_[vertex_id].weight;
    vertices_[vertex_id].weight = weight;
    total_weight_ += vertices_[vertex_id].weight;
};

void AdjacencyListGraph::set_unweighted()
{
    for (VertexId vertex_id = 0;
            vertex_id < number_of_vertices();
            ++vertex_id) {
        set_weight(vertex_id, 1);
    }
}

EdgeId AdjacencyListGraph::add_edge(VertexId vertex_id_1, VertexId vertex_id_2)
{
    if (vertex_id_1 == vertex_id_2) {
        return -1;
    }

    EdgeId edge_id = edges_.size();

    Edge edge;
    edge.vertex_id_1 = vertex_id_1;
    edge.vertex_id_2 = vertex_id_2;
    edges_.push_back(edge);

    VertexEdge ve1;
    ve1.edge_id = edge_id;
    ve1.vertex_id = vertex_id_2;
    vertices_[vertex_id_1].edges.push_back(ve1);
    vertices_[vertex_id_1].neighbors.push_back(vertex_id_2);

    VertexEdge ve2;
    ve2.edge_id = edge_id;
    ve2.vertex_id = vertex_id_1;
    vertices_[vertex_id_2].edges.push_back(ve2);
    vertices_[vertex_id_2].neighbors.push_back(vertex_id_1);

    number_of_edges_++;
    if (maximum_degree_ < std::max(degree(vertex_id_1), degree(vertex_id_2)))
        maximum_degree_ = std::max(degree(vertex_id_1), degree(vertex_id_2));

    return edge_id;
}

void AdjacencyListGraph::clear()
{
    vertices_.clear();
    edges_.clear();
    number_of_edges_ = 0;
    maximum_degree_ = 0;
    total_weight_ = 0;
}

void AdjacencyListGraph::clear_edges()
{
    edges_.clear();
    maximum_degree_ = 0;
    number_of_edges_ = 0;
    for (VertexId vertex_id = 0;
            vertex_id < number_of_vertices();
            ++vertex_id) {
        vertices_[vertex_id].edges.clear();
        vertices_[vertex_id].neighbors.clear();
    }
}

void AdjacencyListGraph::remove_duplicate_edges()
{
    std::vector<std::vector<VertexId>> neighbors(number_of_vertices());
    for (VertexId vertex_id = 0;
            vertex_id < number_of_vertices();
            ++vertex_id) {
        for (VertexId vertex_id_neighbor: vertices_[vertex_id].neighbors)
            if (vertex_id_neighbor > vertex_id)
                neighbors[vertex_id].push_back(vertex_id_neighbor);
        sort(neighbors[vertex_id].begin(), neighbors[vertex_id].end());
        neighbors[vertex_id].erase(
                std::unique(
                    neighbors[vertex_id].begin(),
                    neighbors[vertex_id].end()),
                neighbors[vertex_id].end());
    }
    clear_edges();
    for (VertexId vertex_id_1 = 0;
            vertex_id_1 < number_of_vertices();
            ++vertex_id_1) {
        for (VertexId vertex_id_2: neighbors[vertex_id_1])
            add_edge(vertex_id_1, vertex_id_2);
    }
}

AdjacencyListGraph::AdjacencyListGraph(VertexId number_of_vertices)
{
    for (VertexId vertex_id = 0;
            vertex_id < number_of_vertices;
            ++vertex_id) {
        add_vertex();
    }
}

AdjacencyListGraph AdjacencyListGraph::complementary() const
{
    AdjacencyListGraph graph(number_of_vertices());
    optimizationtools::IndexedSet neighbors(number_of_vertices());
    for (VertexId vertex_id = 0;
            vertex_id < number_of_vertices();
            ++vertex_id) {
        neighbors.clear();
        for (auto it = neighbors_begin(vertex_id);
                it != neighbors_end(vertex_id);
                ++it) {
            neighbors.add(*it);
        }
        for (auto it = neighbors.out_begin(); it != neighbors.out_end(); ++it)
            if (*it > vertex_id)
                graph.add_edge(vertex_id, *it);
    }
    return graph;
}

AdjacencyListGraph::AdjacencyListGraph(
        const AbstractGraph& abstract_graph)
{
    for (VertexId vertex_id = 0;
            vertex_id < abstract_graph.number_of_vertices();
            ++vertex_id) {
        add_vertex();
    }
    for (VertexId vertex_id = 0;
            vertex_id < abstract_graph.number_of_vertices();
            ++vertex_id) {
        for (auto it = abstract_graph.neighbors_begin(vertex_id);
                it != abstract_graph.neighbors_end(vertex_id);
                ++it) {
            if (vertex_id > *it)
                add_edge(vertex_id, *it);
        }
    }
}

void AdjacencyListGraph::read_dimacs1992(std::ifstream& file)
{
    std::string tmp;
    std::vector<std::string> line;

    while (getline(file, tmp)) {
        line = optimizationtools::split(tmp, ' ');
        if (line.size() == 0) {
        } else if (line[0] == "c") {
        } else if (line[0] == "p") {
            VertexId number_of_vertices = stol(line[2]);
            for (VertexId vertex_id = 0;
                    vertex_id < number_of_vertices;
                    ++vertex_id) {
                add_vertex();
            }
        } else if (line[0] == "n") {
            VertexId vertex_id = stol(line[1]) - 1;
            Weight weight = stol(line[2]);
            set_weight(vertex_id, weight);
        } else if (line[0] == "e") {
            VertexId vertex_id_1 = stol(line[1]) - 1;
            VertexId vertex_id_2 = stol(line[2]) - 1;
            add_edge(vertex_id_1, vertex_id_2);
        }
    }
}

void AdjacencyListGraph::read_dimacs2010(std::ifstream& file)
{
    std::string tmp;
    std::vector<std::string> line;
    bool first = true;
    VertexId vertex_id = -1;
    while (vertex_id != number_of_vertices()) {
        getline(file, tmp);
        //std::cout << tmp << std::endl;
        line = optimizationtools::split(tmp, ' ');
        if (tmp[0] == '%')
            continue;
        if (first) {
            VertexId number_of_vertices = stol(line[0]);
            for (VertexId vertex_id = 0;
                    vertex_id < number_of_vertices;
                    ++vertex_id) {
                add_vertex();
            }
            first = false;
            vertex_id = 0;
        } else {
            for (std::string str: line) {
                VertexId vertex_id_2 = stol(str) - 1;
                if (vertex_id_2 > vertex_id)
                    add_edge(vertex_id, vertex_id_2);
            }
            vertex_id++;
        }
    }
}

void AdjacencyListGraph::read_matrixmarket(std::ifstream& file)
{
    std::string tmp;
    std::vector<std::string> line;
    do {
        getline(file, tmp);
    } while (tmp[0] == '%');
    std::stringstream ss(tmp);
    VertexId number_of_vertices = -1;
    ss >> number_of_vertices;
    for (VertexId vertex_id = 0;
            vertex_id < number_of_vertices;
            ++vertex_id) {
        add_vertex();
    }

    VertexId vertex_id_1 = -1;
    VertexId vertex_id_2 = -1;
    while (getline(file, tmp)) {
        std::stringstream ss(tmp);
        ss >> vertex_id_1 >> vertex_id_2;
        add_edge(vertex_id_1 - 1, vertex_id_2 - 1);
    }
}

void AdjacencyListGraph::read_chaco(std::ifstream& file)
{
    std::string tmp;
    std::vector<std::string> line;

    getline(file, tmp);
    line = optimizationtools::split(tmp, ' ');
    VertexId number_of_vertices = stol(line[0]);
    for (VertexId vertex_id = 0;
            vertex_id < number_of_vertices;
            ++vertex_id) {
        add_vertex();
    }

    for (VertexId vertex_id = 0;
            vertex_id < number_of_vertices;
            ++vertex_id) {
        getline(file, tmp);
        line = optimizationtools::split(tmp, ' ');
        for (std::string str: line) {
            VertexId vertex_id_2 = stol(str) - 1;
            if (vertex_id_2 > vertex_id)
                add_edge(vertex_id, vertex_id_2);
        }
    }
}

void AdjacencyListGraph::read_snap(std::ifstream& file)
{
    std::string tmp;
    std::vector<std::string> line;
    do {
        getline(file, tmp);
    } while (tmp[0] == '#');

    VertexId vertex_id_1 = -1;
    VertexId vertex_id_2 = -1;
    for (;;) {
        file >> vertex_id_1 >> vertex_id_2;
        if (file.eof())
            break;
        while (std::max(vertex_id_1, vertex_id_2) >= number_of_vertices())
            add_vertex();
        add_edge(vertex_id_1, vertex_id_2);
    }
}

void AdjacencyListGraph::write(
        std::string instance_path,
        std::string format)
{
    std::ofstream file(instance_path);
    if (!file.good())
        throw std::runtime_error(
                "Unable to open file \"" + instance_path + "\".");

    if (format == "dimacs") {
        write_dimacs(file);
    } else if (format == "matrixmarket") {
        write_matrixmarket(file);
    } else if (format == "snap") {
        write_snap(file);
    } else {
        throw std::invalid_argument(
                "Unknown instance format \"" + format + "\".");
    }
}

void AdjacencyListGraph::write_snap(std::ofstream& file)
{
    for (EdgeId edge_id = 0; edge_id < number_of_edges(); ++edge_id)
        file << first_end(edge_id) << " " << second_end(edge_id) << std::endl;
}

void AdjacencyListGraph::write_matrixmarket(std::ofstream& file)
{
    file << number_of_vertices()
        << " " << number_of_vertices()
        << " " << number_of_edges()
        << std::endl;
    for (EdgeId edge_id = 0; edge_id < number_of_edges(); ++edge_id) {
        file << first_end(edge_id) + 1
            << " " << second_end(edge_id) + 1
            << std::endl;
    }
}

void AdjacencyListGraph::write_dimacs(std::ofstream& file)
{
    file << "p edge " << number_of_vertices() << " " << number_of_edges() << std::endl;
    for (EdgeId edge_id = 0; edge_id < number_of_edges(); ++edge_id) {
        file << "e " << first_end(edge_id) + 1
            << " " << second_end(edge_id) + 1
            << std::endl;
    }
}
