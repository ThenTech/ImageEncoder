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
    enum class ImageSetting : uint8_t {
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

    enum class VideoSetting : uint8_t {
        rawfile = 0,
        encfile,
        decfile,
        rle,
        quantfile,
        width,
        height,
        logfile,
        gop,
        merange,
        motioncompensation,
        AMOUNT
    };

    const std::string SettingToKey(ImageSetting s);
    const std::string SettingToKey(VideoSetting s);

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
            bool getKeyValue(const ImageSetting &key, std::string &value);
            bool getKeyValue(const VideoSetting &key, std::string &value);
            const std::string getValue(const ImageSetting &key) const;
            const std::string getValue(const VideoSetting &key) const;
            const std::string toString(void) const;
            void clear(void);
            bool verifyForImage(void);
            bool verifyForVideo(void);

            std::string getErrorDescription(void) const;
    };
}

#endif // CONFIGREADER_H

