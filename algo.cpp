#include "algo.hpp"

#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include "utils.hpp"

#include <cmath>
#include <cassert>

#if   defined(ALGO_USE_DCT_LEE)
// From: https://www.nayuki.io/page/fast-discrete-cosine-transform-algorithms

// DCT type II, unscaled. Algorithm by Byeong Gi Lee, 1984.
// See:  http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.118.3056&rep=rep1&type=pdf#page=34
static void forwardTransformDCT(double vec[], double temp[], size_t len) {
    if (len == 1)
        return;

    const size_t halfLen = len / 2;

    for (size_t i = 0; i < halfLen; i++) {
        const double x = vec[i];
        const double y = vec[len - 1 - i];

        temp[i] = x + y;
        temp[i + halfLen] = (x - y) / (std::cos((i + 0.5) * M_PI / len) * 2);
    }

    forwardTransformDCT(temp, vec, halfLen);
    forwardTransformDCT(&temp[halfLen], vec, halfLen);

    for (size_t i = 0; i < halfLen - 1; i++) {
        vec[i * 2 + 0] = temp[i];
        vec[i * 2 + 1] = temp[i + halfLen] + temp[i + halfLen + 1];
    }

    vec[len - 2] = temp[halfLen - 1];
    vec[len - 1] = temp[len - 1];
}

// DCT type III, unscaled. Algorithm by Byeong Gi Lee, 1984.
// See: https://www.nayuki.io/res/fast-discrete-cosine-transform-algorithms/lee-new-algo-discrete-cosine-transform.pdf
static void inverseTransformDCT(double vec[], double temp[], size_t len) {
    if (len == 1)
        return;

    size_t halfLen = len / 2;
    temp[0] = vec[0];
    temp[halfLen] = vec[1];

    for (size_t i = 1; i < halfLen; i++) {
        temp[i] = vec[i * 2];
        temp[i + halfLen] = vec[i * 2 - 1] + vec[i * 2 + 1];
    }

    inverseTransformDCT(temp, vec, halfLen);
    inverseTransformDCT(&temp[halfLen], vec, halfLen);

    for (size_t i = 0; i < halfLen; i++) {
        const double x = temp[i];
        const double y = temp[i + halfLen] / (std::cos((i + 0.5) * M_PI / len) * 2);
        vec[i] = x + y;
        vec[len - 1 - i] = x - y;
    }
}

void algo::transformDCT(double vec[], size_t len) {
    assert(len > 0 && (len & (len - 1)) == 0); // Length is power of 2
    std::vector<double> temp(len);
    forwardTransformDCT(vec, temp.data(), len);
}

void algo::transformDCTinverse(double vec[], size_t len) {
    assert(len > 0 && (len & (len - 1)) == 0); // Length is power of 2
    vec[0] /= 2.0;
    std::vector<double> temp(len);
    inverseTransformDCT(vec, temp.data(), len);
}
#elif defined(ALGO_USE_DCT_NAIVE)
/**********************************************
 *  Naive approach, standard from wiki.
 **********************************************/
// From: https://www.nayuki.io/page/fast-discrete-cosine-transform-algorithms

void algo::transformDCT(double vec[], size_t len) {
    std::vector<double> temp(len);
    const double factor = M_PI / len;

    for (size_t i = 0; i < len; i++) {
        double sum = 0;

        for (size_t j = 0; j < len; j++) {
            sum += vec[j] * std::cos((j + 0.5) * i * factor);
        }

        temp[i] = sum;
    }

    std::copy(temp.begin(), temp.end(), vec);
}

void algo::transformDCTinverse(double vec[], size_t len) {
    std::vector<double> temp(len);
    const double factor = M_PI / len;

    for (size_t i = 0; i < len; i++) {
        double sum = vec[0] / 2.0;

        for (size_t j = 1; j < len; j++) {
            sum += vec[j] * std::cos(j * (i + 0.5) * factor);
        }

        temp[i] = sum;
    }

    std::copy(temp.begin(), temp.end(), vec);
}
#else
/**********************************************
 *  Naive approach from iPython notebook.
 **********************************************/
double C(size_t i) {
//    return i == 0 ? M_SQRT2 / 2.0 : 1.0;  // Wrong?
    return i == 0 ? 0.5 : M_SQRT1_2; // voor size=4 of len=16
}

void algo::transformDCT(double vec[], size_t len) {
    double *temp        = util::allocArray<double>(len);
    const size_t size   = size_t(std::sqrt(len));
    const double factor = M_PI_2 / double(size);

    for (size_t u = 0; u < size; u++) {
        for (size_t v = 0; v < size; v++) {
            for (size_t i = 0; i < size; i++) {
                for (size_t j = 0; j < size; j++) {
                    temp[u * size + v] += std::cos(double(2.0 * i + 1.0) * u * factor)
                                        * std::cos(double(2.0 * j + 1.0) * v * factor)
                                        * vec[i * size + j];
                }
            }

//            temp[u * size + v] *= C(u) * C(v) / 2.0;  // Wrong? Why extra div by 4?
            temp[u * size + v] *= C(u) * C(v);
        }
    }

    std::copy_n(temp, len, vec);
    util::deallocArray(temp);
}

void algo::transformDCTinverse(double vec[], size_t len) {
    double *temp        = util::allocArray<double>(len);
    const size_t size   = size_t(std::sqrt(len));
    const double factor = M_PI_2 / double(size);

    for (size_t u = 0; u < size; u++) {
        for (size_t v = 0; v < size; v++) {
            for (size_t i = 0; i < size; i++) {
                for (size_t j = 0; j < size; j++) {
                    temp[i * size + j] += C(u) * C(v) // / 4.0 // Wrong? Why extra div by 4?
                                        * std::cos(double(2.0 * i + 1.0) * u * factor)
                                        * std::cos(double(2.0 * j + 1.0) * v * factor)
                                        * vec[u * size + v];
                }
            }
        }
    }

    std::copy_n(temp, len, vec);
    util::deallocArray(temp);
}
#endif
