#ifndef ALGO_HPP
#define ALGO_HPP

#include <vector>

// Use one implementation of:
//#define ALGO_USE_DCT_LEE
//#define ALGO_USE_DCT_NAIVE
#define ALGO_USE_DCT_NAIVE_PY

namespace algo {
    void transformDCT(double vec[], size_t len);
    void transformDCTinverse(double vec[], std::size_t len);
}

#endif // ALGO_HPP
