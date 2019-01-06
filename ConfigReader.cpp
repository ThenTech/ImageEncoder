#include "ConfigReader.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "utils.hpp"

#define MAXLEN 16384

/**
 *  @brief  Return a string representation of the given Setting.
 *  @params s
 *      The Setting to return a string for.
 *  @return
 *      A string representation for s.
 */
const std::string dc::SettingToKey(dc::ImageSetting s) {
    static const std::string keys[] = {
        "rawfile"  , "encfile", "decfile", "rle",
        "quantfile", "width"  , "height" , "logfile"
    };

    return keys[util::to_underlying(s)];
}

const std::string dc::SettingToKey(dc::VideoSetting s) {
    static const std::string keys[] = {
        "gop", "merange", "motioncompensation"
    };

    const uint8_t idx = util::to_underlying(s);
    const uint8_t off = util::to_underlying(dc::ImageSetting::AMOUNT);

    return idx < off
            ? dc::SettingToKey(dc::ImageSetting(s))
            : keys[idx - off];
}

/**
 * @brief ReadInputLine
 * @param fi
 * @param line
 * @return
 */
static bool ReadInputLine(FILE *fi, std::string &line) {
    if (fi == nullptr) {
		return false;
    }

	char str[MAXLEN];

    if (std::fgets(str, MAXLEN - 1, fi) == nullptr) {
		return false;
    }

    str[MAXLEN - 1] = '\0';
    size_t len = std::strlen(str);

    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
		len--;
        str[len] = '\0';
	}

	line = std::string(str);
	return true;
}



dc::ConfigReader::ConfigReader() {}

dc::ConfigReader::~ConfigReader() {}

bool dc::ConfigReader::read(const std::string &fileName) {
    this->clear();

    #ifdef _MSC_VER
        FILE *pFile;
        fopen_s(&pFile, fileName.c_str(), "rt");
    #else
        FILE *pFile = std::fopen(fileName.c_str(), "rt");
    #endif

    if (pFile == nullptr) {
        this->m_errStr = std::string("Can't open file");
		return false;
	}

	std::string line;

    while (ReadInputLine(pFile, line)) {
        if (line.length() > 0) {
            const size_t s = line.find("=");

            if (s == std::string::npos) {
                this->m_errStr = std::string("Can't find '=' in line");
                std::fclose(pFile);
				return false;
			}

            const std::string key   = line.substr(0, s);
            const std::string value = line.substr(s + 1);

            if (key.length() == 0) {
                this->m_errStr = std::string("Detected an empty key");
                std::fclose(pFile);
				return false;
			}

            if (this->m_keyValues.find(key) != this->m_keyValues.end()) {
                this->m_errStr = std::string("Key '" + key + "' was found more than once!");
                std::fclose(pFile);
				return false;
			}

            this->m_keyValues[key] = value;
		}
	}

    std::fclose(pFile);
	return true;
}

bool dc::ConfigReader::getKeyValue(const ImageSetting &key, std::string &value) {
    std::map<std::string, std::string>::const_iterator it;

    it = this->m_keyValues.find(dc::SettingToKey(key));

    if (it == this->m_keyValues.end()) {
        this->m_errStr = std::string("Key not found: '" + dc::SettingToKey(key) + "'.");
		return false;
	}

	value = it->second;

	return true;
}

bool dc::ConfigReader::getKeyValue(const VideoSetting &key, std::string &value) {
    std::map<std::string, std::string>::const_iterator it;

    it = this->m_keyValues.find(dc::SettingToKey(key));

    if (it == this->m_keyValues.end()) {
        this->m_errStr = std::string("Key not found: '" + dc::SettingToKey(key) + "'.");
        return false;
    }

    value = it->second;

    return true;
}

const std::string dc::ConfigReader::getValue(const ImageSetting &key) const {
    auto it = this->m_keyValues.find(dc::SettingToKey(key));

    return it == this->m_keyValues.end() ? "" : it->second;
}

const std::string dc::ConfigReader::getValue(const VideoSetting &key) const {
    auto it = this->m_keyValues.find(dc::SettingToKey(key));

    return it == this->m_keyValues.end() ? "" : it->second;
}

const std::string dc::ConfigReader::toString(void) const {
    std::ostringstream oss;

    for (const auto& kv : this->m_keyValues) {
        oss << std::setw(18) << kv.first << " = " << kv.second << std::endl;
    }

    return oss.str();
}

void dc::ConfigReader::clear(void) {
    this->m_keyValues.clear();
}

std::string dc::ConfigReader::getErrorDescription(void) const {
    return this->m_errStr;
}

bool dc::ConfigReader::verifyForImage(void) {
    const size_t amount = util::to_underlying(dc::ImageSetting::AMOUNT);

    if (this->m_keyValues.size() != amount) {
        this->m_errStr = std::string("Too many or too few settings in file for image en/decoder!");
        return false;
    }

    std::string err, v;

    for (size_t s = 0; s < amount; s++) {
        if (!this->getKeyValue(dc::ImageSetting(s), v)) {
            err += this->m_errStr + '\n';
        }
    }

    if (err.size() > 0) {
        this->m_errStr = err;
        return false;
    }

    return true;
}

bool dc::ConfigReader::verifyForVideo(bool encoder=true) {
    std::string err, v;

    if (encoder) {
        if (this->m_keyValues.size() < dc::EXPECTED_VideoEncoderSettings) {
            this->m_errStr = std::string("Too many or too few settings in file for video encoder!");
            return false;
        }

        for (size_t s = 0; s < dc::EXPECTED_VideoEncoderSettings; s++) {
            if (!this->getKeyValue(dc::VideoEncoderSettings[s], v)) {
                err += this->m_errStr + '\n';
            }
        }
    } else {
        if (this->m_keyValues.size() < dc::EXPECTED_VideoDecoderSettings) {
            this->m_errStr = std::string("Too many or too few settings in file for video decoder!");
            return false;
        }

        for (size_t s = 0; s < dc::EXPECTED_VideoDecoderSettings; s++) {
            if (!this->getKeyValue(dc::VideoDecoderSettings[s], v)) {
                err += this->m_errStr + '\n';
            }
        }
    }

    if (err.size() > 0) {
        this->m_errStr = err;
        return false;
    }

    return true;
}
