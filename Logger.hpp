#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>

namespace util {
    class Logger {
        private:
            bool enabled;
            std::ofstream log_file;

            static Logger instance;
        public:
            Logger() : enabled(false) {}
            Logger(Logger const&)         = delete;
            void operator=(Logger const&) = delete;

            static void Create(const std::string &fileName);
            static void Destroy();
            static void Write(const std::string &text, bool timestamp=true);
    };
}

#endif // LOGGER_HPP
