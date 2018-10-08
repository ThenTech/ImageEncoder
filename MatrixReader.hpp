#ifndef MATRIXREADER_HPP
#define MATRIXREADER_HPP

#include <string>
#include <cstdint>

#include "Block.hpp"
#include "BitStream.hpp"


namespace dc {
    template<size_t size = dc::BlockSize>
    class MatrixReader {
        private:
            uint32_t matrix  [size * size];
            double   expanded[size * size];
            std::string m_errStr;

            MatrixReader(uint32_t *matrix);

        public:
            MatrixReader(void);
            ~MatrixReader(void);

            static MatrixReader<> fromBitstream(util::BitStreamReader &reader);
            bool read(const std::string &fileName);
            void write(util::BitStreamWriter &writer) const;
            const std::string toString(void) const;

            uint8_t getMaxBitLength(void) const;
            const double* getData(void) const;

            static constexpr size_t SIZE_LEN_BITS = 5;
    };

    extern template class dc::MatrixReader<dc::BlockSize>;
}

#endif // MATRIXREADER_HPP
