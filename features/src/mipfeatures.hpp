// Basic design idea is the MIPInstance class wraps a CoinModel to prevent
// any resource leaks when accessing instance data. For any slightly more
// complex processing, template functions are used with an instance as an
// argument.
//
// This is not very efficient code... there is a lot of copying and several
// of the feature calculation functions access the CoinModel instance, which
// is probably not ideal. But it's workable for now.

#ifndef MIP_INSTANCE_HPP
#define MIP_INSTANCE_HPP

#include <random>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <sstream>
#include <iostream>
#include <random>

#include "gsl/gsl_assert"
#include "gsl/span"

#include <coin/CoinModel.hpp>
#include <coin/ClpSimplex.hpp>
#include "Eigen/Dense"

#include "graph.hpp"

// #define PRINTSOLCHECKS
// using std::cout; using std::endl;

namespace mipfeatures {

const double ROUNDING_TOLERANCE = 0.00001;

// MIP instance class providing slightly nicer interfaces than Coin.
class MIPInstance {

    std::unique_ptr<CoinModel> model;

 public:

    MIPInstance() {}
    MIPInstance(std::unique_ptr<CoinModel> m) : model(move(m)) {}
    MIPInstance(CoinModel* m) : model(m) {}

    // Model size accessors.
    uint numberColumns() const { return model->numberColumns(); }
    uint numberRows() const { return model->numberRows(); }
    uint numberNonZeros() const { return model->numberElements(); }

    // Get variable (objective, is_integer, lower, upper).
    std::tuple<double, bool, double, double> getVar(int index) const {
        return std::make_tuple(
            model->getColObjective(index),
            model->getColIsInteger(index),
            model->getColLower(index),
            model->getColUpper(index));
    }

    // Get row tuple (columns, elements, lower, upper).
    std::tuple<std::vector<int>, std::vector<double>, double, double> getRow(int index) const {
        int nElements = model->getRow(index, nullptr, nullptr);
        std::vector<int> columns(nElements);
        std::vector<double> elements(nElements);
        model->getRow(index, columns.data(), elements.data());
        return std::make_tuple(columns, elements, model->getRowLower(index), model->getRowUpper(index));
    }

    // Span view of the objective vector.
    gsl::span<const double> getObjective() const {
        return gsl::span<const double>{model->objectiveArray(), model->numberColumns()};
    }

    std::vector<double> getRHS() const {
        std::vector<double> rhs;
        for (uint i = 0; i < numberRows(); i++) {
            if (model->getRowUpper(i) >= COIN_DBL_MAX) {
                rhs.push_back(model->getRowLower(i));
            } else {
                rhs.push_back(model->getRowUpper(i));
            }
        }
        return rhs;
    }

    // Return a reference to the underlying coin model.
    const CoinModel& getCoinModel() const {
        return *model;
    }

    // Write this model to MPS as a minimisation model.
    int writeMPS(std::string filename) const {
        return model->writeMps(filename.c_str());
    }

    // Read MPS
    static MIPInstance readMPS(std::string filename) {
        return MIPInstance(new CoinModel(filename.c_str()));
    }

};


template <typename T>
double getSingleLpVarBound(const T& instance, int index, bool upper) {

    CoinModel m(instance.getCoinModel());
    for (int i = 0; i < m.numberColumns(); i++) {
        m.setColumnObjective(i, 0.0);
    }
    if (upper) {
        m.setColumnObjective(index, -1.0);
    } else {
        m.setColumnObjective(index, 1.0);
    }

    ClpModel solver;
    solver.loadProblem(m);
    ClpSimplex simplex(solver);
    simplex.setLogLevel(0);
    simplex.dual();

    if (simplex.isProvenOptimal()) {
        const double * primals = simplex.getColSolution();
        return primals[index];
    }
    if (upper) {
        return COIN_DBL_MAX;
    } else {
        return -COIN_DBL_MAX;
    }

}


template <typename T>
ClpSimplex getClpSimplexModel(const T& instance) {
    CoinModel m(instance.getCoinModel());
    ClpModel solver;
    solver.loadProblem(m);
    ClpSimplex simplex(solver);
    simplex.setLogLevel(0);
    return simplex;
}


void setSingleVarObjective(ClpSimplex& simplex, int index, bool upper) {
    for (int i = 0; i < simplex.numberColumns(); i++) {
        simplex.setObjectiveCoefficient(i, 0.0);
    }
    if (upper) {
        simplex.setObjectiveCoefficient(index, -1.0);
    } else {
        simplex.setObjectiveCoefficient(index, 1.0);
    }
}


template <typename T>
std::vector<std::pair<double, double>> getLpPolyhedralBoundsFast(const T& instance) {
    auto simplex = getClpSimplexModel(instance);
    std::vector<std::pair<double, double>> result;
    for (uint i = 0; i < instance.numberColumns(); i++) {
        // Lower
        setSingleVarObjective(simplex, i, false);
        simplex.primal();
        if (simplex.isProvenOptimal()) {
            result.emplace_back(simplex.getColSolution()[i], COIN_DBL_MAX);
        } else {
            result.emplace_back(-COIN_DBL_MAX, COIN_DBL_MAX);
        }
    }
    for (uint i = 0; i < instance.numberColumns(); i++) {
        // Upper
        setSingleVarObjective(simplex, i, true);
        simplex.primal();
        if (simplex.isProvenOptimal()) {
            result[i].second = simplex.getColSolution()[i];
        }
    }
    return result;
}


template <typename T>
std::vector<std::pair<double, double>> getLpPolyhedralBounds(const T& instance) {
    std::vector<std::pair<double, double>> result;
    result.reserve(instance.numberColumns());
    for (uint i = 0; i < instance.numberColumns(); i++) {
        result.emplace_back(getSingleLpVarBound(instance, i, false), getSingleLpVarBound(instance, i, true));
    }
    return result;
}


// Feature calculation - smallest to largest lattice width.
template <typename T>
double calculatePolyhedralFlatness(const T& instance) {
    double minRange = COIN_DBL_MAX, maxRange = 0;
    auto bounds = getLpPolyhedralBounds(instance);
    for (const auto& bound : bounds) {
        double val = bound.second - bound.first;
        if (val < minRange) { minRange = val; }
        if (val > maxRange) { maxRange = val; }
    }
    return minRange / maxRange;
}


enum LPResult { optimal, infeasible, unbounded, unknown };


template <typename T>
std::pair<LPResult, std::vector<double>> getRelaxedLpSolution(const T& instance) {
    CoinModel m(instance.getCoinModel());
    ClpModel solver;
    solver.loadProblem(m);
    ClpSimplex simplex(solver);
    simplex.setLogLevel(0);
    simplex.dual();
    std::vector<double> result;
    if (simplex.isProvenOptimal()) {
        const double * solution = simplex.getColSolution();
        for (auto i = 0; i < m.numberColumns(); i++) {
            result.push_back(solution[i]);
        }
        return std::make_pair(LPResult::optimal, result);
    } else if (simplex.isProvenPrimalInfeasible()) {
        return std::make_pair(LPResult::infeasible, result);
    } else if (simplex.isProvenDualInfeasible()) {
        return std::make_pair(LPResult::unbounded, result);
    }
    return std::make_pair(LPResult::unknown, result);
}


template <typename T>
std::tuple<LPResult, uint, double, uint, uint> calculateIntegerViolations(const T& instance) {
    const auto [res, lpopt] = getRelaxedLpSolution(instance);
    if (res == LPResult::optimal) {
        uint integer_violations = 0, feasible_down_locks = 0, feasible_up_locks = 0;
        double total_fractionality = 0.0;
        const auto& model = instance.getCoinModel();
        // cout << "LPSOL ";
        for (uint i = 0; i < instance.numberColumns(); i++) {
            const auto& primal = lpopt[i];
            // cout << primal;
            if (model.getColIsInteger(i)) {
                // cout << "I";
                const double ifloor = std::floor(primal);
                if (std::abs(primal - ifloor) > ROUNDING_TOLERANCE) {
                    // cout << "V";
                    integer_violations += 1;
                    total_fractionality += std::min(primal - ifloor, ifloor + 1 - primal);
                    auto x = lpopt;
                    x[i] = ifloor;
                    if (isLpFeasible(instance, x)) { feasible_down_locks += 1; }
                    x[i] = ifloor + 1;
                    if (isLpFeasible(instance, x)) { feasible_up_locks += 1; }
                }
            }
            // cout << " ";
        }
        // cout << endl;
        return std::make_tuple(
            res,                    // LP has a solution
            integer_violations,     // How many integer vars have fractional values
            total_fractionality,
            feasible_down_locks,    // # variables can be feasibly rounded down
            feasible_up_locks       // # variables can be feasibly rounded up
            );
    }
    // Features can't be calculated in this case.
    return std::make_tuple(res, 0, 0.0, 0, 0);
}


// auto [cont, vint, bin] = get_variable_type_counts(instance)
template <typename T>
std::tuple<uint, uint, uint> getVariableTypeCounts(const T& instance) {
    auto result = std::make_tuple<uint, uint, uint>(0, 0, 0);
    for (uint i = 0; i < instance.numberColumns(); i++) {
        auto [obj, is_int, lower, upper] = instance.getVar(i);
        if (is_int) {
            if ((lower == 0) && (upper == 1)) {
                std::get<2>(result) += 1;
            }
            std::get<1>(result) += 1;
        } else {
            std::get<0>(result) += 1;
        }
    }
    return result;
}


template <typename T>
std::vector<uint> getVariableDegrees(const T& instance) {
    std::vector<uint> result;
    auto model = instance.getCoinModel();
    for (uint i = 0; i < instance.numberColumns(); i++) {
        result.push_back(model.getRow(i, nullptr, nullptr));
    }
    return result;
}


template <typename T>
std::vector<uint> getConstraintDegrees(const T& instance) {
    std::vector<uint> result;
    auto model = instance.getCoinModel();
    for (uint i = 0; i < instance.numberRows(); i++) {
        result.push_back(model.getColumn(i, nullptr, nullptr));
    }
    return result;
}


template <typename T>
std::vector<double> getElements(const T& instance) {
    std::vector<double> result;
    for (uint i = 0; i < instance.numberRows(); i++) {
        auto [columns, elements, lower, upper] = instance.getRow(i);
        for (auto element : elements) {
            result.push_back(element);
        }
    }
    return result;
}


template <typename T>
Eigen::MatrixXd getEigenLHSMatrix(const T& instance) {
    Eigen::MatrixXd m(instance.numberRows(), instance.numberColumns());
    m = Eigen::MatrixXd::Zero(instance.numberRows(), instance.numberColumns());
    for (uint i = 0; i < instance.numberRows(); i++) {
        auto [columns, elements, lower, upper] = instance.getRow(i);
        for (uint j = 0; j < columns.size(); j++) {
            m(i, columns[j]) = elements[j];
        }
    }
    return m;
}


template <typename T>
std::pair<double, double> calculateSvdRange(const T& instance) {
    Eigen::MatrixXd m = getEigenLHSMatrix(instance);
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(m);
    const auto& values = svd.singularValues();
    double largest = values[0];
    double smallest = values[values.size() - 1];
    for (uint i = values.size() - 1; i > 0; i--) {
        if (values[i] > ROUNDING_TOLERANCE) {
            smallest = values[i];
            break;
        }
    }
    return std::make_pair(smallest, largest);
}


template <typename T>
bool isLpFeasible(const T& instance, const std::vector<double>& primals) {
    #ifdef PRINTSOLCHECKS
    cout << "CHECKSOL ";
    #endif
    Eigen::MatrixXd A = getEigenLHSMatrix(instance);
    Eigen::MatrixXd x(instance.numberColumns(), 1);
    for (uint i = 0; i < instance.numberColumns(); i++) {
        #ifdef PRINTSOLCHECKS
        cout << primals[i] << " ";
        #endif
        x(i, 0) = primals[i];
    }
    const auto result = A * x;
    // cout << "==X==" << x << endl;
    // cout << "==RES==" << result << endl;
    // cout << "==START==" << endl << A << "==END==" << endl;
    auto rhs = instance.getRHS();
    // cout << endl << "CONS ";
    for (uint j = 0; j < instance.numberRows(); j++) {
        // cout << result(j, 0) - rhs[j] << " ";
        if (result(j, 0) - rhs[j] > ROUNDING_TOLERANCE) {
            #ifdef PRINTSOLCHECKS
            cout << "INF" << endl;
            #endif
            return false;
        }
    }
    #ifdef PRINTSOLCHECKS
    cout << "FEAS" << endl;
    #endif
    return true;
}


template <typename T>
double sampleRoundings(const T& instance, uint samples, std::default_random_engine& rng) {
    std::bernoulli_distribution distribution(0.5);
    double count = 0.0;
    const auto [res, lpopt] = getRelaxedLpSolution(instance);
    if (res == LPResult::optimal) {
        const auto& model = instance.getCoinModel();
        for (uint k = 0; k < samples; k++) {
            auto x = lpopt;
            for (uint i = 0; i < instance.numberColumns(); i++) {
                if (model.getColIsInteger(i)) {
                    if (std::abs(lpopt[i] - std::floor(lpopt[i])) > ROUNDING_TOLERANCE) {
                        if (distribution(rng)) {
                            x[i] = std::ceil(lpopt[i]);
                        } else {
                            x[i] = std::floor(lpopt[i]);
                        }
                    }
                }
            }
            if (isLpFeasible(instance, x)) {
                count += 1.0;
            }
        }
        return count / samples;
    }
    return 0;
}


template <typename T>
graph::UndirectedGraph getGraph(const T& instance) {
    std::vector<std::pair<int, int>> edges;
    for (uint i = 0; i < instance.numberRows(); i++) {
        auto [columns, elements, lower, upper] = instance.getRow(i);
        for (uint j = 0; j < columns.size(); j++) {
            // Edge from variable to constraint.
            edges.emplace_back(i, columns[j] + instance.numberRows());
        }
    }
    graph::UndirectedGraph g(instance.numberRows() + instance.numberColumns());
    g.add_edges(edges);
    return g;
}


class GraphFeatures {
public:
    GraphFeatures(const graph::UndirectedGraph& g) {
        girth = graph::girth(g);
        clustering_coefficient = graph::clustering_coefficient(g);
        std::tie( szeged_index, revised_szeged_index ) = graph::szeged_indices(g);
        std::tie ( energy, adjacency_eigenvalue_stdev, beta ) = adjacency_eigenvalue_stats(g);
        algebraic_connectivity = graph::algebraic_connectivity_lapack_dense(g);
    }

    uint girth;
    double clustering_coefficient;
    double szeged_index;
    double revised_szeged_index;
    double beta;
    double energy;
    double adjacency_eigenvalue_stdev;
    double algebraic_connectivity;
    double eigenvector_centrality_mean;
    double eigenvector_centrality_std;

};


template <typename T>
GraphFeatures vcGraphFeatures(const T& instance) {
    auto g = getGraph(instance);
    return GraphFeatures(g);
}


}


#endif
