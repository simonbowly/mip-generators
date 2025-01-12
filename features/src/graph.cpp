
#include <cmath>
#include <fstream>

#include "gsl/gsl_assert"
#include "graph.hpp"


using namespace std;


namespace graph {


void UndirectedGraph::add_edges(vector<pair<int, int>> edges)
{
    igraphVector v(edges.size() * 2);
    int i = 0;
    for (auto edge : edges) {
        VECTOR(*v.get())[i] = edge.first;
        VECTOR(*v.get())[i+1] = edge.second;
        i += 2;
    }
    igraph_add_edges(graph.get(), v.get(), 0);
}


double density(const UndirectedGraph& graph) {
    double v = graph.vertices();
    double e = graph.edges();
    return 2 * e / (v * (v - 1));
}


bool is_connected(const UndirectedGraph& graph) {
    igraph_bool_t res;
    igraph_is_connected(graph.get(), &res, IGRAPH_STRONG);
    return res;
}


const igraphVector degree(const UndirectedGraph& graph) {
    igraphVector res(graph.vertices());
    /*int ret = */igraph_degree(graph.get(), res.get(), igraph_vss_all(), IGRAPH_ALL, IGRAPH_NO_LOOPS);
    res.update();
    Ensures(res.size() == graph.vertices());
    return res;
}

const igraphVector eigenvector_centrality(const UndirectedGraph& graph) {
    igraphVector res(graph.vertices());
    igraph_arpack_options_t options;
    igraph_arpack_options_init(&options);
    /*int ret = */igraph_eigenvector_centrality(
        graph.get(), res.get(),
		nullptr,        // eigenvalue (not needed)
        false,          // false = undirected
        false,          // false = do not scale
        nullptr,        // nul = unweighted
        &options);
    res.update();
    Ensures(res.size() == graph.vertices());
    return res;
}


const igraphVector adjacency_eigenvalues(const UndirectedGraph& graph) {
    // Get adjacency matrix.
    igraphMatrix adjacency(graph.vertices(), graph.vertices());
    /*int ret = */igraph_get_adjacency(
        graph.get(), adjacency.get(),
        IGRAPH_GET_ADJACENCY_BOTH,  // upper and lower triangular
        false);                     // false = number of edges

    // Calculate all eigenvalues.
    igraphVector res(graph.vertices());
    /*int ret = */igraph_lapack_dsyevr(
        adjacency.get(),
		IGRAPH_LAPACK_DSYEV_ALL,
		0.0, 0.0, 0.0,      // bounds for eigenvalues (only for INTERVAL mode)
		0, 0,               // lower and upper indexing (only for SELECT mode)
        1e-10,              // convergence tolerance
		res.get(),          // resulting eigenvalues
        nullptr,            // eigenvectors are discarded
		nullptr);           // support is discarded
    res.update();

    Ensures(res.size() == graph.vertices());
    return res;
}


const tuple<double, double, double> adjacency_eigenvalue_stats(const UndirectedGraph& graph) {
    // Returns:
    //      energy (mean of absolute values of eigenvalues)
    //      (absolute) eigenvalue standard deviation
    //      beta bipartitivity parameter (even closed walks/all closed walks)

    // Calculate eigenvalues and absolutes.
    const igraphVector eigenvalues = adjacency_eigenvalues(graph);
    vector<double> absolute_eigenvalues;
    absolute_eigenvalues.reserve(graph.vertices());
    for (const auto & v : eigenvalues) {
        absolute_eigenvalues.push_back(fabs(v));
    }
    Ensures(absolute_eigenvalues.size() == (uint) graph.vertices());

    // Mean/stdev statistics.
    auto [energy, stdev] = simple_statistics(absolute_eigenvalues);

    // Beta bipartitivity.
    double sc_even = 0.0;
    double sc_total = 0.0;
    for (const double& eig : eigenvalues) {
        sc_even += cosh(eig);
        sc_total += exp(eig);
    }

    return make_tuple(energy, stdev, sc_even / sc_total);
}


double algebraic_connectivity_lapack_dense(const UndirectedGraph& graph) {
    
    // Short-circuit.
    if (!is_connected(graph)) { return 0; }

    // Get laplacian matrix.
    igraphMatrix laplacian(graph.vertices(), graph.vertices());
    /*int ret = */igraph_laplacian(
        graph.get(), laplacian.get(),
        nullptr,                    // don't create sparse laplacian
        false,                      // false = non-normalised
        nullptr);                   // null = unweighted

    // Must allocate N-length workspace to avoid memory issues. igraph will resize.
    //      ref https://github.com/igraph/igraph/issues/1109
    igraphVector result(graph.vertices());
    /*int ret = */igraph_lapack_dsyevr(
        laplacian.get(),
		IGRAPH_LAPACK_DSYEV_SELECT,
		0.0, 0.0, 0.0,      // bounds for eigenvalues (only for INTERVAL)
		2, 2,               // select second smallest eigenvalue only
        1e-10,              // convergence tolerance
		result.get(),       // resulting eigenvalues
        nullptr,            // eigenvectors are discarded
		nullptr);           // support is discarded
    result.update();

    // Resulting vector has size 1, so begin() points to the result.
    Ensures(result.size() == 1);
    return *result.begin();
}

int multiplier(igraph_real_t *to, const igraph_real_t *from, int n, void *extra) {

    const igraph_matrix_t* A = (igraph_matrix_t*) extra;

    igraphVector row(n);
    for (int i = 0; i < n; i++) {
        igraph_matrix_get_row(A, row.get(), i);
        row.update();
        to[i] = 0.0;
        for (int j = 0; j < n; j++) {
            to[i] += row[j] * from[j];
        }
    }

    return 0;
}

double algebraic_connectivity_arpack_dense(const UndirectedGraph& graph) {

    // Short-circuit.
    if (!is_connected(graph)) { return 0; }

    // Get laplacian matrix.
    igraphMatrix laplacian(graph.vertices(), graph.vertices());
    /*int ret = */igraph_laplacian(
        graph.get(), laplacian.get(),
        nullptr,                    // don't create sparse laplacian
        false,                      // false = non-normalised
        nullptr);                   // null = unweighted

    // ARPACK configuration for eigenvalue calculation.
    igraph_arpack_options_t options;
    igraph_arpack_options_init(&options);
    options.n = graph.vertices();
    options.which[0]='S'; options.which[1]='A';     // calculate from the small end
    options.nev = 2;                                // get two smallest values
    options.ncv = 0;                                // 0 means "automatic" in igraph_arpack_rssolve
    options.start = 0;		                        // random start vector
    options.mxiter = 10000;                         // iterations to convergence

    // Callback eigenvalue calculation.
    igraphVector values(2);
    igraph_arpack_rssolve(
        multiplier, laplacian.get(),    // Callback multiplying L * x
        &options,
        nullptr,                        // Automatic storage structures.
        values.get(),                   // Eigenvalues.
        nullptr);                       // Eigenvectors not required.
    values.update();

    // Resulting vector has size 2, so begin() + 1 points to the result.
    Ensures(values.size() == 2);
    return *(values.begin() + 1);

}


double wiener_index(const UndirectedGraph& graph) {
    // Simple sum of inter-vertex distances over unordered vertex pairs.
    igraphMatrix res(graph.vertices(), graph.vertices());
    igraph_shortest_paths(graph.get(), res.get(), igraph_vss_all(), igraph_vss_all(), IGRAPH_ALL);

    double result = 0;
    for (int i = 0; i < graph.vertices(); i++) {
        for (int j = i + 1; j < graph.vertices(); j++) {
            result += res.element(i, j);
        }
    }

    return result;
}


const pair<double, double> szeged_indices(const UndirectedGraph& graph) {

    // Edge list.
    igraphVector edge_list(graph.edges() * 2);
    igraph_get_edgelist(graph.get(), edge_list.get(), false);
    edge_list.update();

    // Distance matrix.
    igraphMatrix distance(graph.vertices(), graph.vertices());
    igraph_shortest_paths(graph.get(), distance.get(), igraph_vss_all(), igraph_vss_all(), IGRAPH_ALL);

    double szeged = 0, revised_szeged = 0;

    for (int e = 0; e < graph.edges(); e++) {

        int u = edge_list[e * 2];
        int v = edge_list[e * 2 + 1];
        double n_uv = 0, n_vu = 0, o_uv = 0;

        // Compare distances from all other vertices to u and v.
        for (int i = 0; i < graph.vertices(); i++) {
            if ((i == u) || (i == v)) { continue; }

            if (distance.element(u, i) < distance.element(v, i)) {
                // vertex i is closer to u than v
                n_uv += 1;
            } else if (distance.element(u, i) > distance.element(v, i)) {
                // vertex i is closer to v than u
                n_vu += 1;
            } else {
                // i is equidistant from v and u
                o_uv += 1;
            }
        }

        szeged += n_uv * n_vu;
        revised_szeged += (n_uv + o_uv / 2) * (n_vu + o_uv / 2);

    }

    return make_pair(szeged, revised_szeged);
}


int radius(const UndirectedGraph& graph) {
    igraph_real_t res;
    /*int ret = */igraph_radius(
        graph.get(), &res,
        IGRAPH_ALL);    // igraph_neimode_t mode
    return res;
}

int girth(const UndirectedGraph& graph) {
    igraph_integer_t res;
    /*int ret = */igraph_girth(
        graph.get(), &res,
        nullptr);       // igraph_vector_t *circle
    return res;
}

double clustering_coefficient(const UndirectedGraph& graph) {
    igraph_real_t res;
    /*int ret = */igraph_transitivity_undirected(
        graph.get(), &res,
        IGRAPH_TRANSITIVITY_ZERO);  // igraph_transitivity_mode_t mode
    return res;
}


UndirectedGraph read_dimacs(string file_name) {

    uint vertices = 0, edges = 0;
    vector<pair<int, int>> edge_list;

    string line;
    ifstream col_file(file_name);
    if (col_file.is_open()) {
        while ( getline(col_file, line) ) {
            if (line.substr(0, 1).compare("p") == 0) {
                // assert vertices, edges are 0
                string info = line.substr(7, line.size());
                auto found = info.find(" ");
                vertices = stoi(info.substr(0, found));
                edges = stoi(info.substr(found + 1, info.size()));
                edge_list.reserve(edges);
            } else if (line.substr(0, 1).compare("e") == 0) {
                // assert vertices, edges are not 0
                string info = line.substr(2, line.size());
                auto found = info.find(" ");
                int a = stoi(info.substr(0, found));
                int b = stoi(info.substr(found + 1, info.size()));
                edge_list.emplace_back(a - 1, b - 1);
            }
        }
    } else {
        throw "File not open.";
    }

    if (edge_list.size() != edges) {
        throw "Incorrect number of edges.";
    }

    UndirectedGraph g(vertices);
    g.add_edges(edge_list);
    return g;

}


UndirectedGraph random_tree(int vertices, int children) {
    auto g = impl::create_igraph_ptr();
    igraph_tree(g.get(), vertices, children, IGRAPH_TREE_UNDIRECTED);
    return UndirectedGraph(g);
}


UndirectedGraph random_bipartite(int n1, int n2, double p) {
    auto g = impl::create_igraph_ptr();
    igraph_bipartite_game(g.get(), nullptr, IGRAPH_ERDOS_RENYI_GNP, n1, n2, p, 0, false, IGRAPH_ALL);
    return UndirectedGraph(g);
}


UndirectedGraph erdos_renyi_gnm(int n, int m) {
    auto g = impl::create_igraph_ptr();
    igraph_erdos_renyi_game(g.get(), IGRAPH_ERDOS_RENYI_GNM, n, m, false, false);
    return UndirectedGraph(g);
}


UndirectedGraph erdos_renyi_gnp(int n, double p) {
    auto g = impl::create_igraph_ptr();
    igraph_erdos_renyi_game(g.get(), IGRAPH_ERDOS_RENYI_GNP, n, p, false, false);
    return UndirectedGraph(g);
}


}
