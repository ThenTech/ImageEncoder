#ifndef MATRIXREADER_HPP
#define MATRIXREADER_HPP

#include <string>
#include <cstdint>


namespace dc {
    static constexpr size_t DefaultMatrixSize = 4u;

    template<size_t size>
    class MatrixReader {
        private:
            uint16_t matrix[size][size];
            std::string m_errStr;

        public:
            MatrixReader(void);
            ~MatrixReader(void);

            bool read(const std::string &fileName);
            const std::string toString(void) const;
    };

    extern template class dc::MatrixReader<dc::DefaultMatrixSize>;
}

#endif // MATRIXREADER_HPP
