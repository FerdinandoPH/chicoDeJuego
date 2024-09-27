#include <emu.h>
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
#endif
int main(int argc, char **argv) {
    #ifdef _WIN32
        if(argc<2){
            char *filename = open_file_dialog();
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
    #endif
    return emuRun(argc, argv);
}