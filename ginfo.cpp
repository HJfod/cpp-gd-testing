#include <iostream>
#include <string>
#include <ShlObj.h>
#include <windows.h>
#include <sstream>
#include <Combaseapi.h>

// clang ginfo.cpp -o gd.exe -lshell32 -lole32

using namespace std;

string GET_CC(string WHICH = "LocalLevels") {
    wchar_t* localAppData = 0;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppData);

    wstring CCW (localAppData);

    string RESULT ( CCW.begin(), CCW.end() );
    RESULT += "\\GeometryDash\\CC" + WHICH + ".dat";

    CoTaskMemFree(static_cast<void*>(localAppData));
    
    return RESULT;
}

int main(int ARGC, char *ARGS[]) {
    if (ARGC > 1) {
        cout << "Loading info for " << ARGC-1 << " level(s)... \n";

        // load CCLocalLevels path
        string CCPATH = GET_CC();

        int i;
        for (i = 0; i < ARGC - 1; i++) {
            string LEVEL_NAME = ARGS[i+1];

            // xor
        }

        cout << "\n---Finished! :)---\n";
    } else {
        cout << "You need to supply a level name to run this exe.";
    }

    return 0;
}