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

#define APP_VERSION "v1.0"

// BUILD COMMAND: clang ginfo.cpp ext/ZlibHelper.cpp -o gd.exe -L"." -lshell32 -lole32 -lzlib -m32 -std=c++17 -O3

// RUN COMMAND: ./ginfo <level_name> <level_name> <level_name> ...

std::string REPL( std::string const& original, std::string const& from, std::string const& to ) {
    std::string results;
    std::string::const_iterator end = original.end();
    std::string::const_iterator current = original.begin();
    std::string::const_iterator next = std::search( current, end, from.begin(), from.end() );
    while ( next != end ) {
        results.append( current, next );
        results.append( to );
        current = next + from.size();
        next = std::search( current, end, from.begin(), from.end() );
    }
    results.append( current, next );
    return results;
}

std::vector<uint8_t> READ_FILE(std::string const& path) {
    std::vector<uint8_t> buffer;
    std::ifstream file(path, std::ios::ate, std::ios::binary);

    if (file.is_open()) {
        buffer.resize(file.tellg());
        file.seekg(0, std::ios::beg);

        file.read(
            reinterpret_cast<char*>(buffer.data()),
            buffer.size());
    }

    return buffer;
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

constexpr unsigned int HASH(const char* str, int h = 0) {
    return !str[h] ? 5381 : (HASH(str, h+1) * 33) ^ str[h];
}

std::vector<std::string> GET_LEVELS(const std::string DATA) {
    std::regex m ("<k>k_[0-9]+<\\/k>.*?<\\/d>.*?<\\/d>");
    std::smatch sm;

    std::string RED = DATA;
    std::vector<std::string> LIST (1);

    int i = 0;
    while (std::regex_search(RED, sm, m)) {
        LIST.resize(i + 1);
        LIST[i] = sm[0];

        i++;

        RED = RED.substr(sm[0].length() - 5);
    }

    return LIST;
}

std::string REPL_O_SONG(int ID) {
    std::string arr[21] = {
        "Stereo Madness",
        "Back on Track",
        "Polargeist",
        "Dry Out",
        "Base After Base",
        "Cant Let Go",
        "Jumper",
        "Time Machine",
        "Cycles",
        "xStep",
        "Clutterfunk",
        "Theory of Everything",
        "Electronman Adventures",
        "Clubstep",
        "Electrodynamix",
        "Hexagon Force",
        "Blast Processing",
        "Theory of Everything 2",
        "Geometrical Dominator",
        "Deadlocked",
        "Fingerdash"
    };
    return ID <= 20 && ID >= 0 ? arr[ID] : "Unknown";
}

std::string REPL_LENGTH(int L) {
    std::string LGTS[5] = {
        "Tiny",
        "Short",
        "Medium",
        "Long",
        "XL"
    };
    return L >= 0 && L <= 4 ? LGTS[L] : "Unknown";
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

std::string GET_PROP(const std::string DATA, std::string PROP) {
    // basically an abstracted shorthand for GET_KEY

    std::string RES;
    std::string WORKSTR;
    std::vector<uint8_t> WORKVCT;
    long WORKINT;

    switch (HASH(PROP.c_str())) {
        case HASH("NAME"):
            RES = GET_KEY(DATA, "k2");
            break;
        case HASH("CREATOR"):
            RES = GET_KEY(DATA, "k5");
            break;
        case HASH("DATA"):
            RES = GET_KEY(DATA, "k4");
            break;
        case HASH("LENGTH"):
            WORKSTR = GET_KEY(DATA, "k23");
            RES = (WORKSTR == "" ? "Tiny" : REPL_LENGTH(std::stoi(WORKSTR)));
            break;
        case HASH("VERSION"):
            RES = GET_KEY(DATA, "k16");
            break;
        case HASH("PASSWORD"):
            WORKSTR = GET_KEY(DATA, "k41");
            RES = (WORKSTR == "1" || WORKSTR == "" ? "Free to Copy" : WORKSTR == "0" ? "No Copy" : WORKSTR);
            break;
        case HASH("SONG"):
            WORKSTR = GET_KEY(DATA, "k8");
            RES = (WORKSTR != "" ? REPL_O_SONG(std::stoi(WORKSTR)) : GET_KEY(DATA, "k45") == "" ? "Unknown" : GET_KEY(DATA, "k45"));
            break;
        case HASH("DESCRIPTION"):
            WORKVCT = DECODE_BASE64(GET_KEY(DATA, "k3"));
            WORKSTR = std::string(WORKVCT.data(), WORKVCT.data() + WORKVCT.size());
            RES = (WORKSTR == "" ? "None" : WORKSTR);
            break;
        case HASH("OBJECT COUNT"):
            RES = GET_KEY(DATA, "k48");
            break;
        case HASH("EDITOR TIME"):
            WORKINT = std::stol(GET_KEY(DATA, "k80"));
            RES = (WORKINT > 3600 ? std::to_string((int)(WORKINT / 3600.0)) + "h" : std::to_string((int)(WORKINT / 60.0)) + "m" );
            break;
        case HASH("VERIFIED"):
            RES = GET_KEY(DATA, "k14", "");
            break;
        case HASH("ATTEMPTS"):
            WORKSTR = GET_KEY(DATA, "k18");
            RES = (WORKSTR == "" ? "None" : WORKSTR);
            break;
        case HASH("REVISION"):
            WORKSTR = GET_KEY(DATA, "k46");
            RES = (WORKSTR == "" ? "None" : WORKSTR);
            break;
        case HASH("COPY"):
            WORKSTR = GET_KEY(DATA, "k42");
            RES = (WORKSTR == "" ? "None" : WORKSTR);
            break;
        default:
            RES = "Unknown property.";
    }

    return RES;
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
            LEVEL_NAME = REPL(LEVEL_NAME, "_", " ");

            std::cout << std::endl << "--- Info on " << LEVEL_NAME << " ---" << std::endl;

            std::vector<std::string> LVLS = GET_LEVELS(ZLIB);

            std::string LVL = "";

            for (int i = 0; i < LVLS.size(); i++) {
                if (GET_KEY(LVLS[i], "k2") == LEVEL_NAME) {
                    LVL = LVLS[i];
                }
            }

            if (LVL == "") {
                std::cout << std::endl << "Could not find level! (Replace spaces in name with _)" << std::endl;
            } else {
                std::cout << std::endl << "Name: \t\t" <<        GET_PROP(LVL, "NAME");
                std::cout << std::endl << "Creator: \t" <<       GET_PROP(LVL, "CREATOR");
                std::cout << std::endl << "Length: \t" <<        GET_PROP(LVL, "LENGTH");
                std::cout << std::endl << "Version: \t" <<       GET_PROP(LVL, "VERSION");
                std::cout << std::endl << "Password: \t" <<      GET_PROP(LVL, "PASSWORD");
                std::cout << std::endl << "Song: \t\t" <<        GET_PROP(LVL, "SONG");
                std::cout << std::endl << "Description: \t" <<   GET_PROP(LVL, "DESCRIPTION");
                std::cout << std::endl << "Object count: \t" <<  GET_PROP(LVL, "OBJECT COUNT");
                std::cout << std::endl << "Editor time: \t" <<   GET_PROP(LVL, "EDITOR TIME");
                std::cout << std::endl << "Verified: \t" <<      GET_PROP(LVL, "VERIFIED");
                std::cout << std::endl << "Attempts: \t" <<      GET_PROP(LVL, "ATTEMPTS");
                std::cout << std::endl << "Revision: \t" <<      GET_PROP(LVL, "REVISION");
                std::cout << std::endl << "Copied from: \t" <<   GET_PROP(LVL, "COPY");
            }

            std::cout << std::endl << std::endl;
        }

        std::cout << "\n--- Finished! :) ---\n";
    } else {
        std::cout << "You need to supply a level name to run this exe.";
    }

    return 0;
}