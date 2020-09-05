#include <iostream>
#include <string>
#include <ShlObj.h>
#include <windows.h>
#include <sstream>
#include <Combaseapi.h>
#include <fstream>
#include <vector>
#include <assert.h>
#include <regex>
#include "ext/ZlibHelper.hpp"
#include "ext/Base64.hpp"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

// RUN COMMAND: clang ginfo.cpp ext/ZlibHelper.cpp -o gd.exe -L"." -lshell32 -lole32 -lzlib -m32 -std=c++17; ./gd.exe moi

bool REPL(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::vector<uint8_t> READ_FILE(std::string const& path) {
    std::ifstream file(path, std::ios::binary);

    if (file.is_open()) {
        return std::vector<uint8_t>(std::istreambuf_iterator(file), {});
    }

    return {};
}

std::string GET_CC(std::string WHICH = "LocalLevels") {
    wchar_t* localAppData = 0;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppData);

    std::wstring CCW (localAppData);

    std::string RESULT ( CCW.begin(), CCW.end() );
    RESULT += "\\GeometryDash\\CC" + WHICH + ".dat";

    CoTaskMemFree(static_cast<void*>(localAppData));
    
    return RESULT;
}

void DECODE_XOR(std::vector<uint8_t>& BYTES, int KEY) {
    for (auto& b : BYTES)
        b ^= KEY;
}

std::string DECODE_BASE64(const std::string& str) {
    gdcrypto::base64::Base64 b64(gdcrypto::base64::URL_SAFE_DICT);
    auto buffer = b64.decode(std::vector<uint8_t>(str.begin(), str.end()));
    return std::string(buffer.data(), buffer.data() + buffer.size());
}

std::string DECOMPRESS_GZIP(const std::string& str) {
    auto buffer = gdcrypto::zlib::inflateBuffer(std::vector<uint8_t>(str.begin(), str.end()));
    return std::string(buffer.data(), buffer.data() + buffer.size());
}

std::string GET_KEY(std::string DATA, std::string KEY, std::string TYPE = ".*?") {
    if (TYPE == "") {
        std::regex m ("<k>" + KEY + "</k>");
        return (std::regex_search(DATA, m)) ? "True" : "False";
    } else {
        std::regex m ("<k>" + KEY + "</k><" + TYPE + ">");
        std::match_results<const char*> cm;
        std::regex_search(DATA, cm, m, std::regex_constants::match_default);

        std::cout << "the matches were: ";
        for (unsigned i=0; i<cm.size(); ++i) {
            std::cout << "[" << cm[i] << "] ";
        }
    }
}

int main(int ARGC, char *ARGS[]) {
    if (ARGC > 1) {
        std::cout << "Loading info for " << ARGC-1 << " level(s)... \n";

        // load CCLocalLevels path
        std::string CCPATH = GET_CC();
        std::vector<uint8_t> CCCONTENTS = READ_FILE(CCPATH);

        DECODE_XOR(CCCONTENTS, 11);
        auto XOR = std::string(CCCONTENTS.begin(), CCCONTENTS.end());
        std::string B64 = DECODE_BASE64(XOR);
        std::string ZLIB = DECOMPRESS_GZIP(B64);

        std::cout << "Decoded CCLocalLevels...";

        for (int i = 0; i < ARGC - 1; i++) {
            std::string LEVEL_NAME = ARGS[i+1];
        }

        std::cout << "\n---Finished! :)---\n";
    } else {
        std::cout << "You need to supply a level name to run this exe.";
    }

    return 0;
}