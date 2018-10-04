#include "ConfigReader.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "utils.hpp"

#define MAXLEN 16384

const std::string dc::SettingToKey(dc::Setting s) {
    static const std::string keys[] = {
        "rawfile"  , "encfile", "decfile", "rle",
        "quantfile", "width"  , "height" , "logfile"
    };

    return keys[uint8_t(s)];
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

    if (fgets(str, MAXLEN - 1, fi) == nullptr) {
		return false;
    }

    str[MAXLEN - 1] = '\0';
    size_t len = strlen(str);

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

	FILE *pFile = fopen(fileName.c_str(), "rt");

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
				fclose(pFile);
				return false;
			}

            const std::string key   = line.substr(0, s);
            const std::string value = line.substr(s + 1);

            if (key.length() == 0) {
                this->m_errStr = std::string("Detected an empty key");
				fclose(pFile);
				return false;
			}

            if (this->m_keyValues.find(key) != this->m_keyValues.end()) {
                this->m_errStr = std::string("Key '" + key + "' was found more than once!");
				fclose(pFile);
				return false;
			}

            this->m_keyValues[key] = value;
		}
	}

	fclose(pFile);
	return true;
}

bool dc::ConfigReader::getKeyValue(const Setting &key, std::string &value) {
    std::map<std::string, std::string>::const_iterator it;

    it = this->m_keyValues.find(dc::SettingToKey(key));

    if (it == this->m_keyValues.end()) {
        this->m_errStr = std::string("Key not found: '" + dc::SettingToKey(key) + "'.");
		return false;
	}

	value = it->second;

	return true;
}

const std::string dc::ConfigReader::getValue(const Setting &key) const {
    auto it = this->m_keyValues.find(dc::SettingToKey(key));

    return it == this->m_keyValues.end() ? "" : it->second;
}

const std::string dc::ConfigReader::toString(void) const {
    std::ostringstream oss;

    for (const auto& kv : this->m_keyValues) {
        oss << std::setw(10) << kv.first << " = " << kv.second << std::endl;
    }

    return oss.str();
}

void dc::ConfigReader::clear(void) {
    this->m_keyValues.clear();
}

std::string dc::ConfigReader::getErrorDescription(void) const {
    return this->m_errStr;
}

bool dc::ConfigReader::verify(void) {
    const size_t amount = util::to_underlying(dc::Setting::AMOUNT);

    if (this->m_keyValues.size() != amount) {
        this->m_errStr = std::string("Too many or too few settings in file!");
        return false;
    }

    std::string err, v;

    for (size_t s = 0; s < amount; s++) {
        if (!this->getKeyValue(Setting(s), v)) {
            err += this->m_errStr + '\n';
        }
    }

    if (err.size() > 0) {
        this->m_errStr = err;
        return false;
    }

    return true;
}
