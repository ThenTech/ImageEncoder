#ifndef IMAGEBASE_HPP
#define IMAGEBASE_HPP

#include <string>
#include <vector>

#include "BitStream.hpp"

namespace dc {
    /**
     *  @brief  The ImageBase class
     */
    class ImageBase {
        protected:
            std::vector<uint8_t>  *raw;
            util::BitStreamReader *reader;
        public:
            ImageBase(const std::string &source_file);
            ~ImageBase(void);
    };

    ////////////////////////////////////////////////////////////////////////////////////

    /**
     *  @brief  The ImageProcessor class
     */
    class ImageProcessor : protected ImageBase {
        protected:
            const std::string     &dest_file;
            util::BitStreamWriter *writer;
        public:
            ImageProcessor(const std::string &source_file, const std::string &dest_file);
            virtual ~ImageProcessor(void);

            virtual bool process(void);
            virtual void saveResult(void) const;
    };
}

#endif // IMAGEBASE_HPP
