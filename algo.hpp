#ifndef ALGO_HPP
#define ALGO_HPP

#include <vector>

namespace algo {
    // From: https://www.nayuki.io/page/fast-discrete-cosine-transform-algorithms

    void transformDCT(std::vector<double> &vec);
    void transformDCT(double vec[], size_t len);
    void naiveTransformDCT(double vec[], size_t len);
    void transformDCTinverse(std::vector<double> &vec);
    void transformDCTinverse(double vec[], std::size_t len);
    void naiveTransformDCTinverse(double vec[], size_t len);
}

#endif // ALGO_HPP
