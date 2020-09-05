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
#include <chrono>
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

#define APP_VERSION "v0.11"

// RUN COMMAND: clang ginfo.cpp ext/ZlibHelper.cpp -o gd.exe -L"." -lshell32 -lole32 -lzlib -m32 -std=c++17 -O3; ./gd.exe moi

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

std::vector<uint8_t> DECODE_BASE64(const std::string& str) {
    gdcrypto::base64::Base64 b64(gdcrypto::base64::URL_SAFE_DICT);
    return b64.decode(str);
}

std::string DECOMPRESS_GZIP(const std::vector<uint8_t> str) {
    auto buffer = gdcrypto::zlib::inflateBuffer(str);
    return std::string(buffer.data(), buffer.data() + buffer.size());
}

std::string GET_KEY(const std::string DATA, std::string KEY, std::string TYPE = ".*?") {
    if (TYPE == "") {
        std::regex m ("<k>" + KEY + "</k>");
        return (std::regex_search(DATA, m)) ? "True" : "False";
    } else {
        std::regex m ("<k>" + KEY + "</k><" + TYPE + ">");
        std::smatch cm;
        std::regex_search(DATA, cm, m);

        if (cm[0] == "") return "";

        std::string T_TYPE = ((std::string)cm[0]).substr(((std::string)cm[0]).find_last_of("<") + 1, 1);

        std::regex tm ("<k>" + KEY + "</k><" + T_TYPE + ">.*?</" + T_TYPE + ">");
        std::smatch tcm;
        std::regex_search(DATA, tcm, tm);

        std::string VAL = tcm[0];

        int L1 = ("<k>" + KEY + "</k><" + T_TYPE + ">").length();
        return VAL.substr(L1, VAL.find_last_of("</") - L1 - 1);
    }
}

std::string * GET_LEVELS(const std::string DATA) {
    std::regex m ("<k>k_\\d+<\\/k>.+?<\\/d>\\n? *<\\/d>");
    std::smatch sm;
    std::regex_search(DATA, sm, m);

    std::string * LIST = new std::string [sm.size() + 1];

    for (unsigned int i = 0; i < sm.size(); i++) {
        LIST[i] = sm[i];
    }

    return LIST;
}

int main(int ARGC, char *ARGS[]) {
    if (ARGC > 1) {
        std::cout << "GD Level Info Tool " << APP_VERSION << std::endl << std::endl;
        std::cout << "Loading info for " << ARGC-1 << " level(s)... \n";

        auto TIME_MEASURE = std::chrono::high_resolution_clock::now();

        // load CCLocalLevels
        std::string CCPATH = GET_CC();
        std::vector<uint8_t> CCCONTENTS = READ_FILE(CCPATH);

        DECODE_XOR(CCCONTENTS, 11);
        auto XOR = std::string(CCCONTENTS.begin(), CCCONTENTS.end());
        std::vector<uint8_t> B64 = DECODE_BASE64(XOR);
        std::string ZLIB = DECOMPRESS_GZIP(B64);

        std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - TIME_MEASURE;
        std::cout << "Decoded CCLocalLevels in " << elapsed.count() * 1000 << "ms" << std::endl;

        // loop through args
        for (int i = 0; i < ARGC - 1; i++) {
            std::string LEVEL_NAME = ARGS[i+1];

            std::cout << std::endl << "--- Info on " << LEVEL_NAME << " ---" << std::endl;

            std::string * LVLS = GET_LEVELS(ZLIB);

            std::string LVL = "";

            for (int i = 0; i < 50; i++) {
                if (GET_KEY(LVLS[i], "k2") == LEVEL_NAME) {
                    LVL = LVLS[i];
                }
            }

            if (LVL == "") {
                std::cout << "Could not find level!" << std::endl;
                return 1;
            }

            std::cout << GET_KEY(LVL, "k5");
        }

        std::cout << "\n---Finished! :)---\n";
    } else {
        std::cout << "You need to supply a level name to run this exe.";
    }

    return 0;
}