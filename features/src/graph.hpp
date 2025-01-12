
#ifndef GRAPH_HPP
#define GRAPH_HPP


#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "igraph/igraph.h"

#include "utils.hpp"


namespace graph {


    namespace impl {

        // cleanup handler for an igraph_t pointer with associated resources

        struct del_igraph_t {
            void operator()(igraph_t* p) const {
                igraph_destroy(p);
                delete p;
            }
        };

        typedef std::unique_ptr<igraph_t, del_igraph_t> igraph_ptr;

        inline igraph_ptr create_igraph_ptr() {
            return impl::igraph_ptr(new igraph_t(), impl::del_igraph_t());
        }

        inline igraph_ptr create_igraph_empty(int n) {
            igraph_ptr graph = impl::create_igraph_ptr();
            igraph_empty(graph.get(), n, IGRAPH_UNDIRECTED);
            return graph;
        }

        inline igraph_ptr create_igraph_copy(const igraph_ptr& other) {
            igraph_ptr graph = impl::create_igraph_ptr();
            igraph_copy(graph.get(), other.get());
            return graph;
        }

    }


    class UndirectedGraph {

        impl::igraph_ptr graph;

     public:

        UndirectedGraph() { graph = impl::create_igraph_empty(0); }
        UndirectedGraph(impl::igraph_ptr& g) { graph = move(g); }
        explicit UndirectedGraph(int n) { graph = impl::create_igraph_empty(n); }
        UndirectedGraph(const UndirectedGraph& a) { impl::create_igraph_copy(a.graph); }
        UndirectedGraph(UndirectedGraph&& a) noexcept { graph = move(a.graph); }
        ~UndirectedGraph() {}

        UndirectedGraph& operator=(const UndirectedGraph& a) {
            graph = impl::create_igraph_copy(a.graph);
            return *this;
        }

        UndirectedGraph& operator=(UndirectedGraph&& a) noexcept {
            graph = move(a.graph);
            return *this;
        }

        // Alterations.
        void add_edge(int from, int to) { igraph_add_edge(graph.get(), from, to); }
        void add_edges(std::vector<std::pair<int, int>> edges);

        // Read only pointer for feature calculations.
        const igraph_t* get() const { return graph.get(); }

        // Basic properties.
        int vertices() const { return igraph_vcount(graph.get()); }
        int edges() const { return igraph_ecount(graph.get()); }

    };

    double density(const UndirectedGraph&);
    bool is_connected(const UndirectedGraph&);

    // Single value features.
    double average_path_length(const UndirectedGraph&);
    int diameter(const UndirectedGraph&);
    int radius(const UndirectedGraph&);
    int girth(const UndirectedGraph&);
    double clustering_coefficient(const UndirectedGraph&);
    double algebraic_connectivity_arpack_dense(const UndirectedGraph&);
    double algebraic_connectivity_lapack_dense(const UndirectedGraph&);
    double wiener_index(const UndirectedGraph&);

    // Szeged, Revised Szeged index pair.
    const std::pair<double, double> szeged_indices(const UndirectedGraph&);

    // Vertex and eigenvalue properties.
    const igraphVector degree(const UndirectedGraph&);
    const igraphVector betweenness_centrality(const UndirectedGraph&);
    const igraphVector eigenvector_centrality(const UndirectedGraph&);
    const igraphVector adjacency_eigenvalues(const UndirectedGraph&);

    // Energy, stdev, beta bipartitivity tuple.
    const std::tuple<double, double, double> adjacency_eigenvalue_stats(const UndirectedGraph&);

    UndirectedGraph read_dimacs(std::string);
    UndirectedGraph random_tree(int vertices, int children);
    UndirectedGraph random_bipartite(int n1, int n2, double p);
    UndirectedGraph erdos_renyi_gnm(int n, int m);
    UndirectedGraph erdos_renyi_gnp(int n, double p);

}


#endif
