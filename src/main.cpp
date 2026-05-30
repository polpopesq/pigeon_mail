#ifdef _WIN32
  #include <windows.h>
#endif

#include "PigeonMailApp.h"

int main() {
#ifdef _WIN32
    // Enable UTF-8 output and ANSI escape codes on Windows 10+
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode))
            SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif

    PigeonMailApp app;
    app.run();
    return 0;
}
