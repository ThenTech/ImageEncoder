import qbs

Project {
    minimumQbsVersion: "1.7.1"


    CppApplication {
        consoleApplication: true
        files: [
            "BitStream.cpp",
            "BitStream.hpp",
            "ConfigReader.cpp",
            "ConfigReader.hpp",
            "Decoder.cpp",
            "Decoder.hpp",
            "Encoder.cpp",
            "Encoder.hpp",
            "Exceptions.hpp",
            "ImageBase.cpp",
            "ImageBase.hpp",
            "Logger.cpp",
            "Logger.hpp",
            "MatrixReader.cpp",
            "MatrixReader.hpp",
            "main.cpp",
            "main.hpp",
            "matrix.txt",
            "settings.conf",
            "utils.hpp",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: "application"
            qbs.install: true
        }

        cpp.optimization: "none"
        cpp.cxxLanguageVersion: "c++17"
        cpp.warningLevel: "all"
        cpp.debugInformation: true
    }
}
