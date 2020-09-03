#include <iostream>
#include <string>
#include <ShlObj.h>
#include <windows.h>
#include <sstream>
#include <Combaseapi.h>
#include <fstream>
#include <vector>

#include "boost/iostreams/filtering_streambuf.hpp"
#include "boost/iostreams/copy.hpp"
#include "boost/iostreams/filter/gzip.hpp"

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

string DECOMPRESS_GZIP(const string& DATA) {
    using namespace boost::iostreams::zlib;
    using namespace boost::iostreams;

    stringstream RES;
    
    filtering_streambuf<input> in;
    in.push(gzip_decompressor());
    in.push(STRBYTE(DATA));
    boost::iostreams::copy(in, RES);
    
    return RES.str();
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