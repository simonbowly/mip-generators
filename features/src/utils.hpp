
#ifndef UTILS_HPP
#define UTILS_HPP

#include "math.h"

#include "gsl/span"
#include "igraph/igraph.h"


namespace graph {

    class igraphVector {

        igraph_vector_t v;
        gsl::span<const igraph_real_t> vec_span;   // possible to just inherit from this?

     public:

        // Allocate vector size.
        igraphVector(uint n) {
            igraph_vector_init(&v, n);
            vec_span = gsl::span<const igraph_real_t>{VECTOR(v), igraph_vector_size(&v)};
        }

        // Clean up the igraph memory allocation.
        ~igraphVector() {
                igraph_vector_destroy(&v);
        }

        // No copy operations or move assignments allowed.
        igraphVector(const igraphVector&) = delete;
        igraphVector& operator=(const igraphVector&) = delete;
        igraphVector& operator=(igraphVector&& a) = delete;

        // Move constructor assigns the igraph_vector_t pointer.
        igraphVector(igraphVector&& a) {
            v = a.v;                            a.v = igraph_vector_t();
            vec_span = a.vec_span;              a.vec_span = gsl::span<const igraph_real_t>{};
        }

        // Update the span object in case the underlying memory may have changed.
        // This should be called after any igraph function which might resize the vector.
        void update() {
            vec_span = gsl::span<const igraph_real_t>{VECTOR(v), igraph_vector_size(&v)};
        }

        // Span accessors.
        int size() const { return vec_span.size(); }
        gsl::span<const igraph_real_t>::iterator begin() const { return vec_span.begin(); }
        gsl::span<const igraph_real_t>::iterator end() const { return vec_span.end(); }
        const igraph_real_t& operator[](int i) const { return vec_span[i]; }

        // Return pointer to igraph_vector_t to pass to igraph modifier functions.
        igraph_vector_t* get() { return &v; }

    };

    class igraphMatrix {

        igraph_matrix_t m;

     public:

        igraphMatrix(uint rows, uint cols) {  igraph_matrix_init(&m, rows, cols);  }
        ~igraphMatrix() {  igraph_matrix_destroy(&m);  }

        // No copy or move allowed.
        igraphMatrix(const igraphMatrix&) = delete;
        igraphMatrix& operator=(const igraphMatrix&) = delete;
        igraphMatrix(igraphMatrix&&) = delete;
        igraphMatrix& operator=(igraphMatrix&& a) = delete;

        // Element access.
        igraph_real_t element(int i, int j) const { return MATRIX(m, i, j); }

        // Return igraph_matrix_t pointer to pass to igraph modifier functions.
        igraph_matrix_t* get() { return &m; }

    };

    template<class T>
    std::pair<double, double> simple_statistics(T d) {

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

        return std::make_pair(mean, stdev);
    }

}


#endif
