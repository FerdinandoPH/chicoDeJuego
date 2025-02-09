#include <utils.h>



#ifdef _WIN32

    void createProcess(const char *proc){ //Used for opening the HEX editor
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        char cmd[MAX_PATH];
        snprintf(cmd, MAX_PATH, "cmd.exe /C %s", proc);

        if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            printf("Error running the following command: %s\n", proc);
            return;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#endif