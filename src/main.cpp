#ifdef _WIN32
#include <windows.h>
#endif

#include "App.h"
#include "ConsoleUI.h"

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

    App app;
    ConsoleUI ui;
    app.registerHandlers();  // App listens for Ui* events
    ui.registerHandlers();   // UI listens for App* events — replays any queued
    ui.run();
    return 0;
}
