#include <iostream>

#include "main.hpp"
#include "utils.hpp"
#include "Logger.hpp"

#include "ConfigReader.hpp"
#include "MatrixReader.hpp"

#ifdef ENCODER
    #include "Encoder.hpp"
#endif
#ifdef DECODER
    #include "Decoder.hpp"
#endif

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "One argument, the name of a settings file, expected!" << std::endl;
        return 1;
    }

    dc::ConfigReader c;
    
    if (!c.read(argv[1])) {
        std::cerr << "Error reading file '" << argv[1] << "'!" << std::endl;
        std::cerr << c.getErrorDescription() << std::endl;
        return 2;
    }

    // Enforce existence of all exprected keys.
    if (!c.verify()) {
        std::cerr << "Error in settings!" << std::endl;
        std::cerr << c.getErrorDescription() << std::endl;
        return 3;
    }

    util::Logger::Create(c.getValue(dc::Setting::logfile));

    util::Logger::Write("Input settings:", false);
    util::Logger::Write("-------------------------", false);
    util::Logger::Write(c.toString(), false);

    const std::string encfile = c.getValue(dc::Setting::encfile),
                      decfile = c.getValue(dc::Setting::decfile);
    bool success = true;

    #ifdef ENCODER
        dc::MatrixReader<> m;

        if (!m.read(c.getValue(dc::Setting::quantfile))) {
            return 4;
        }

        util::Logger::Write("Quantization matrix:", false);
        util::Logger::Write("-------------------------", false);
        util::Logger::Write(m.toString(), false);

        const std::string rawfile = c.getValue(dc::Setting::rawfile);
        uint16_t width, height, rle;

        try {
            width  = util::lexical_cast<uint16_t>(c.getValue(dc::Setting::width).c_str());
            height = util::lexical_cast<uint16_t>(c.getValue(dc::Setting::height).c_str());
            rle    = util::lexical_cast<uint16_t>(c.getValue(dc::Setting::rle).c_str());
        } catch (Exceptions::CastingException const& e) {
            util::Logger::Write(e.getMessage());
            return 5;
        }

        dc::Encoder enc(rawfile, encfile, width, height, rle, m);

        if ((success = enc.process())) {
            enc.saveResult();
        } else {
            util::Logger::Write("Error processing raw image for encoding! See log for details.");
        }
    #endif

    #ifdef DECODER
        if (success) {
            dc::Decoder dec(encfile, decfile);

            if (dec.process()) {
                dec.saveResult();
            } else {
                util::Logger::Write("Error processing raw image for decoding! See log for details.");
            }
        }
    #endif

    util::Logger::Destroy();
    return 0;
}
