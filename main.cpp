#include <iostream>

#include "main.hpp"
#include "utils.hpp"
#include "Logger.hpp"

#include "ConfigReader.hpp"
#include "MatrixReader.hpp"

#ifdef ENCODER
    #include "ImageEncoder.hpp"
    #include "VideoEncoder.hpp"
#endif
#ifdef DECODER
    #include "ImageDecoder.hpp"
    #include "VideoDecoder.hpp"
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

    // Enforce existence of all expected keys.
    const bool input_is_image = c.verifyForImage();
    const std::string errstri = c.getErrorDescription();

    const bool input_is_video = c.verifyForVideo();
    const std::string errstrv = c.getErrorDescription();

    bool is_any = false;

    if (input_is_image && !input_is_video) {
        is_any = true;
    } else if (input_is_video && !input_is_image) {
        is_any = true;
    } else {
        std::cerr << "Error in settings!" << std::endl;
        if (errstri.size() > 0) std::cerr << errstri << std::endl;
        if (errstrv.size() > 0) std::cerr << errstrv << std::endl;
        return 3;
    }

    #ifdef LOG_OFF
        util::Logger::Create("");
    #else
        util::Logger::Create(c.getValue(dc::ImageSetting::logfile));
    #endif

    util::Logger::WriteLn("Input settings:", false);
    util::Logger::WriteLn("-------------------------", false);
    util::Logger::WriteLn(c.toString(), false);

    const std::string encfile = c.getValue(dc::ImageSetting::encfile),
                      decfile = c.getValue(dc::ImageSetting::decfile);

    bool success = true;
    util::timepoint_t start = util::TimerStart();

    #ifdef ENCODER
        const std::string rawfile = c.getValue(dc::ImageSetting::rawfile);

        if (rawfile == encfile) {
            std::cerr << "Error in settings! Encoded filename must be different from raw filename!" << std::endl;
            return 3;
        }

        dc::MatrixReader<> m;

        if (!m.read(c.getValue(dc::ImageSetting::quantfile))) {
            return 4;
        }

        util::Logger::WriteLn("Quantization matrix:", false);
        util::Logger::WriteLn("-------------------------", false);
        util::Logger::WriteLn(m.toString(), false);

        uint16_t width, height, rle, gop, merange;

        try {
            width  = util::lexical_cast<uint16_t>(c.getValue(dc::ImageSetting::width).c_str());
            height = util::lexical_cast<uint16_t>(c.getValue(dc::ImageSetting::height).c_str());
            rle    = util::lexical_cast<uint16_t>(c.getValue(dc::ImageSetting::rle).c_str());

            if (input_is_video) {
                gop        = util::lexical_cast<uint16_t>(c.getValue(dc::VideoSetting::gop).c_str());
                merange    = util::lexical_cast<uint16_t>(c.getValue(dc::VideoSetting::merange).c_str());
            }
        } catch (Exceptions::CastingException const& e) {
            util::Logger::WriteLn(e.getMessage());
            return 5;
        }

        if (input_is_image) {
            dc::ImageEncoder enc(rawfile, encfile, width, height, rle, m);

            if ((success = enc.process())) {
                enc.saveResult();

                util::Logger::WriteLn("", false);
                util::Logger::WriteLn(std::string_format("Elapsed time: %f milliseconds",
                                                         util::TimerDuration_ms(start)));
                util::Logger::WriteLn("", false);
                util::Logger::WriteLn("", false);
            } else {
                util::Logger::WriteLn("Error processing raw image for encoding! See log for details.");
            }
        } else if (input_is_video) {
            dc::VideoEncoder enc(rawfile, encfile, width, height, rle, m, gop, merange, true);

            if ((success = enc.process())) {
                enc.saveResult();

                util::Logger::WriteLn("", false);
                util::Logger::WriteLn(std::string_format("Elapsed time: %f milliseconds",
                                                         util::TimerDuration_ms(start)));
                util::Logger::WriteLn("", false);
                util::Logger::WriteLn("", false);
            } else {
                util::Logger::WriteLn("Error processing raw video for encoding! See log for details.");
            }
        }
    #endif

    #ifdef DECODER
        if (encfile == decfile) {
            std::cerr << "Error in settings! Decoded filename must be different from encoded!" << std::endl;
            return 3;
        }

        if (success) {
            start = util::TimerStart();

            if (input_is_image) {
                dc::ImageDecoder dec(encfile, decfile);

                if (dec.process()) {
                    dec.saveResult();

                    util::Logger::WriteLn("", false);
                    util::Logger::WriteLn(std::string_format("Elapsed time: %f milliseconds",
                                                             util::TimerDuration_ms(start)));
                    util::Logger::WriteLn("", false);
                } else {
                    util::Logger::Write("Error processing raw image for decoding! See log for details.");
                }
            } else if (input_is_video) {
                uint16_t motioncomp;

                try {
                    motioncomp = util::lexical_cast<uint16_t>(c.getValue(dc::VideoSetting::motioncompensation).c_str());
                } catch (Exceptions::CastingException const& e) {
                    util::Logger::WriteLn(e.getMessage());
                    return 5;
                }

                dc::VideoDecoder dec(encfile, decfile, motioncomp);

                if (dec.process()) {
                    dec.saveResult();

                    util::Logger::WriteLn("", false);
                    util::Logger::WriteLn(std::string_format("Elapsed time: %f milliseconds",
                                                             util::TimerDuration_ms(start)));
                    util::Logger::WriteLn("", false);
                } else {
                    util::Logger::Write("Error processing raw video for decoding! See log for details.");
                }
            }
        }
    #endif

    util::Logger::Destroy();
    return 0;
}
