#include "algo.hpp"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <cassert>

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

void algo::transformDCT(std::vector<double> &vec) {
    algo::transformDCT(vec.data(), vec.size());
}

void algo::transformDCTinverse(double vec[], size_t len) {
    assert(len > 0 && (len & (len - 1)) == 0); // Length is power of 2
    vec[0] /= 2.0;
    std::vector<double> temp(len);
    inverseTransformDCT(vec, temp.data(), len);
}

void algo::transformDCTinverse(std::vector<double> &vec) {
    algo::transformDCTinverse(vec.data(), vec.size());
}



/**********************************************
 *  Naive approach, standard from wiki.
 **********************************************/

void algo::naiveTransformDCT(double vec[], size_t len) {
    std::vector<double> temp(len);
    const double factor = M_PI / len;

    for (size_t i = 0; i < len; i++) {
        double sum = 0;
        for (size_t j = 0; j < len; j++)
            sum += vec[j] * std::cos((j + 0.5) * i * factor);
        temp[i] = sum;
    }

    std::copy(temp.begin(), temp.end(), vec);
}

void algo::naiveTransformDCTinverse(double vec[], size_t len) {
    std::vector<double> temp(len);
    const double factor = M_PI / len;

    for (size_t i = 0; i < len; i++) {
        double sum = vec[0] / 2;
        for (size_t j = 1; j < len; j++)
            sum += vec[j] * std::cos(j * (i + 0.5) * factor);
        temp[i] = sum;
    }

    std::copy(temp.begin(), temp.end(), vec);
}
