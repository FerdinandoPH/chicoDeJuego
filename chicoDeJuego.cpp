#include "emu.h"
#include <SDL3/SDL_main.h>
#ifdef _WIN32

    #include <windows.h>
    #include <commdlg.h>
    char* open_file_dialog() {
        static char filename[MAX_PATH] = "";

        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = filename;
        ofn.lpstrFile[0] = '\0';
        ofn.nMaxFile = sizeof(filename);
        ofn.lpstrFilter = "All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn)) {
            return filename;
        } else {
            return NULL;
        }
    }
#else
    // Linux / Unix: usamos zenity, que abre un dialogo GTK e imprime la ruta
    // seleccionada por stdout. En Ubuntu suele venir preinstalado; en Raspberry
    // Pi OS puede requerir `sudo apt install zenity`.
    #include <cstdio>
    #include <cstring>
    #include <limits.h>
    char* open_file_dialog() {
        static char filename[PATH_MAX] = "";

        // 2>/dev/null silencia los warnings de GTK que zenity suele soltar.
        FILE* pipe = popen(
            "zenity --file-selection --title=\"Selecciona una ROM\" 2>/dev/null",
            "r");
        if (!pipe) return NULL;

        char* ok = fgets(filename, sizeof(filename), pipe);
        pclose(pipe);
        if (!ok) return NULL;                      // el usuario cancelo

        filename[strcspn(filename, "\n")] = '\0';  // quitar el '\n' final
        return filename[0] ? filename : NULL;
    }
#endif
int main(int argc, char **argv) {
    if(argc<2){
        char* filename = open_file_dialog();
        if (filename != NULL) {
            char **new_argv =(char**) malloc((argc + 1) * sizeof(char *));
            if (new_argv) {
                new_argv[0] = argv[0];
                new_argv[1] = filename;
                argv = new_argv;
                argc++;
            }
        }
    }
    return emu_run(argc, argv);
}