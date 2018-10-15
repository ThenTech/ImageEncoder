#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <string>
#include <cstdint>
#include <map>

namespace dc {
    /**
     *  @brief  An enum with every type of setting.
     *          Use Setting::<name> to reauest a setting from a ConfigReader instance
     *          instead of a raw string.
     */
    enum class Setting : uint8_t {
        rawfile = 0,
        encfile,
        decfile,
        rle,
        quantfile,
        width,
        height,
        logfile,
        AMOUNT
    };

    const std::string SettingToKey(Setting s);

    /**
     *  @brief
     *  Helper class to read config files containing 'key=value' lines.
     *  If a function returns 'false', the getErrorDescription() member
     *  provides an error string
     */
    class ConfigReader {
        private:
            std::map<std::string, std::string> m_keyValues;
            std::string m_errStr;
        public:
            ConfigReader(void);
            ~ConfigReader(void);

            bool read(const std::string &fileName);
            bool getKeyValue(const Setting &key, std::string &value);
            const std::string getValue(const Setting &key) const;
            const std::string toString(void) const;
            void clear(void);
            bool verify(void);

            std::string getErrorDescription(void) const;
    };
}

#endif // CONFIGREADER_H

