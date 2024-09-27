#include <utils.h>

#ifdef _WIN32

    void createProcess(const char *proc){
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Convertir el comando a un array de caracteres modificable
        char cmd[MAX_PATH];
        snprintf(cmd, MAX_PATH, "cmd.exe /C %s", proc);

        // Crear el proceso
        if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            printf("Error ejecutando el comando: %s\n", proc);
            return;
        }

        // Cerrar los handles del proceso y del hilo
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#endif