#include <iostream>
#include <string>
#include <ShlObj.h>
#include <windows.h>
#include <sstream>
#include <Combaseapi.h>
#include <fstream>
#include <vector>
#include "gzip/zlib.h"
#include <assert.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

// clang ginfo.cpp -o gd.exe -lshell32 -lole32

using namespace std;

bool REPL(string& str, const string& from, const string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == string::npos) return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

char* STRBYTE(string S) {
    char* BYTES = new char [S.size() + 1];
    strncpy(BYTES, S.c_str(), sizeof(BYTES));
    return BYTES;
}

string READ_FILE(string FILE) {
    string RESULT;
    ifstream FS (FILE);
    if (FS.is_open()) {
        string LN;
        while ( getline (FS, LN) ) {
            RESULT += LN + "\n";
        }
        FS.close();
    }
    return RESULT;
}

string GET_CC(string WHICH = "LocalLevels") {
    wchar_t* localAppData = 0;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppData);

    wstring CCW (localAppData);

    string RESULT ( CCW.begin(), CCW.end() );
    RESULT += "\\GeometryDash\\CC" + WHICH + ".dat";

    CoTaskMemFree(static_cast<void*>(localAppData));
    
    return RESULT;
}

string DECODE_XOR(string TEXT, int KEY) {
    char* BYTES = STRBYTE(TEXT);

    string RESULT;

    for (int i = 0; i < strlen(BYTES); i++) {
        RESULT += BYTES[i] ^ KEY;
    }

    return RESULT;
}

string DECODE_BASE64(const string &in) {
    string out;

    vector<int> T(256,-1);
    for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i; 

    int val=0, valb=-8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val<<6) + T[c];
        valb += 6;
        if (valb>=0) {
            out.push_back(char((val>>valb)&0xFF));
            valb-=8;
        }
    }
    return out;
}

std::string DECOMPRESS_GZIP(const string& str) {
    #define MOD_GZIP_ZLIB_WINDOWSIZE 15
    #define MOD_GZIP_ZLIB_CFACTOR    9
    #define MOD_GZIP_ZLIB_BSIZE      8096

    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (inflateInit2(&zs, MOD_GZIP_ZLIB_WINDOWSIZE + 16) != Z_OK)
        throw(std::runtime_error("inflateInit failed while decompressing."));

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // get the decompressed bytes blockwise using repeated calls to inflate
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, 0);

        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer,
                             zs.total_out - outstring.size());
        }

    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib decompression: (" << ret << ") "
            << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
}

int main(int ARGC, char *ARGS[]) {
    if (ARGC > 1) {
        cout << "Loading info for " << ARGC-1 << " level(s)... \n";

        // load CCLocalLevels path
        string CCPATH = GET_CC();
        string CCCONTENTS = READ_FILE(CCPATH);

        string XOR = DECODE_XOR(CCCONTENTS, 11);
        REPL(XOR, "_", "/");
        REPL(XOR, "-", "+");
        REPL(XOR, "\0", "");
        int REM = XOR.length() % 4;
        if (REM > 0) for (int r = 0; r < REM; r++) { XOR += "="; }
        string B64 = DECODE_BASE64(XOR);
        string ZLIB = DECOMPRESS_GZIP(B64);

        cout << ZLIB;

        for (int i = 0; i < ARGC - 1; i++) {
            string LEVEL_NAME = ARGS[i+1];
        }

        cout << "\n---Finished! :)---\n";
    } else {
        cout << "You need to supply a level name to run this exe.";
    }

    return 0;
}