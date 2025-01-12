
#include <iostream>
#include <random>

#include "mipfeatures.hpp"

using namespace std;
using namespace mipfeatures;


// auto [vmin, vmax, vmean, vstdev] = simple_statistics( vector-like-thing )
template<class T>
std::tuple<double, double, double, double> simpleStatistics(const T& d) {
    double n = d.size();
    double mean = 0.0;
    for (const auto & vd : d) {
        mean += vd;
    }
    mean /= n;
    double stdev = 0.0;
    for (const auto & vd : d) {
        stdev += (vd - mean) * (vd - mean);
    }
    stdev = sqrt(stdev / (n - 1.0));
    auto [vmin, vmax] = minmax_element(d.begin(), d.end());
    return std::make_tuple(*vmin, *vmax, mean, stdev);
}


template <typename T>
void print_vector_stats(std::string label, const T& vec) {
    auto [vmin, vmax, vmean, vstdev] = simpleStatistics(vec);
    cout << label << "_min: " << vmin << endl;
    cout << label << "_max: " << vmax << endl;
    cout << label << "_mean: " << vmean << endl;
    cout << label << "_stdev: " << vstdev << endl;
}


void print_features(const MIPInstance& m, std::default_random_engine& rng) {
    // Basic size-related metrics.
    cout << "variables: " << m.numberColumns() << endl;
    auto [cont, vint, bin] = getVariableTypeCounts(m);
    cout << "continuous_variables: " << cont << endl;
    cout << "integer_variables: " << vint << endl;
    cout << "binary_variables: " << bin << endl;
    cout << "constraints: " << m.numberRows() << endl;
    cout << "nonzeros: " << m.numberNonZeros() << endl;
    // Relaxation solution characteristics.
    auto [
        lp_result,
        relaxation_integer_violations, relaxation_total_fractionality,
        feasible_down_locks, feasible_up_locks
        ] = calculateIntegerViolations(m);
    switch (lp_result) {
        case LPResult::optimal:
            cout << "lp_result: optimal" << endl;
            cout << "lp_has_solution: true" << endl;
            break;
        case LPResult::infeasible:
            cout << "lp_result: infeasible" << endl;
            cout << "lp_has_solution: false" << endl;
            break;
        case LPResult::unbounded:
            cout << "lp_result: unbounded" << endl;
            cout << "lp_has_solution: false" << endl;
            break;
        case LPResult::unknown:
            cout << "lp_result: unknown" << endl;
            cout << "lp_has_solution: false" << endl;
            break;
    }
    cout << "relaxation_integer_violations: " << relaxation_integer_violations << endl;
    cout << "relaxation_total_fractionality: " << relaxation_total_fractionality << endl;
    // Rounding and sampling solutions.
    cout << "feasible_round_ups: " << feasible_up_locks << endl;
    cout << "feasible_round_downs: " << feasible_down_locks << endl;
    auto feasible_roundings = sampleRoundings(m, 1000, rng);
    cout << "prob_feasible_rounding: " << feasible_roundings << endl;
    // Statistics of various vectors.
    print_vector_stats("objective", m.getObjective());
    print_vector_stats("rhs", m.getRHS());
    print_vector_stats("lhs_coefficient", getElements(m));
    print_vector_stats("variable_degree", getVariableDegrees(m));
    print_vector_stats("constraint_degree", getConstraintDegrees(m));
    // Numerical scaling measures.
    auto [smallest, largest] = calculateSvdRange(m);
    cout << "svd_smallest: " << smallest << endl;
    cout << "svd_largest: " << largest << endl;
    cout << "svd_condition: " << largest / smallest << endl;
    // Variable-constraint graph
    auto graph_features = vcGraphFeatures(m);
    cout << "vc_girth: " << graph_features.girth << endl;
    cout << "vc_clustering_coefficient: " << graph_features.clustering_coefficient << endl;
    cout << "vc_szeged_index: " << graph_features.szeged_index << endl;
    cout << "vc_revised_szeged_index: " << graph_features.revised_szeged_index << endl;
    cout << "vc_beta: " << graph_features.beta << endl;
    cout << "vc_energy: " << graph_features.energy << endl;
    cout << "vc_adjacency_eigenvalue_stdev: " << graph_features.adjacency_eigenvalue_stdev << endl;
    cout << "vc_algebraic_connectivity: " << graph_features.algebraic_connectivity << endl;
    cout << "vc_eigenvector_centrality_mean: " << graph_features.eigenvector_centrality_mean << endl;
    cout << "vc_eigenvector_centrality_std: " << graph_features.eigenvector_centrality_std << endl;
    // Other metrics.
    cout << "lattice_flatness: " << calculatePolyhedralFlatness(m) << endl;
}


int main(int argc, char *argv[]) {

    std::default_random_engine rng(2113585);

    for (int i = 1; i < argc; i++) {
        string instance_file(argv[i]);
        try {
            const MIPInstance m = MIPInstance::readMPS(instance_file);
            cout << "===== " << instance_file << " =====" << endl;
            print_features(m, rng);
        } catch (...) {
            cerr << "Skipped " << instance_file << " due to error" << endl;
        }
    }

    return 0;

}
