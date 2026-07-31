#include "video.h"
#include "sdl_internal.h"
#include <SDL3/SDL.h>

// SDL3 video backend. Everything that used to be tangled inside Ui lives here:
// windows, renderers, textures and the placement of the auxiliary windows.
class Sdl_video : public IVideo {
    private:
        SDL_Window*   main_window   = nullptr;
        SDL_Renderer* main_renderer = nullptr;
        SDL_Texture*  main_texture  = nullptr;
        int main_width  = 0;
        int main_height = 0;

        struct Aux {
            SDL_Window*   window   = nullptr;
            SDL_Renderer* renderer = nullptr;
            SDL_Texture*  texture  = nullptr;
            int height = 0;
            int scale  = 1;
            bool open  = false;
        };
        Aux aux[NUM_DEBUG_WINDOWS];

    public:
        bool init(int width, int height, int scale) override {
            if (!SDL_Init(SDL_INIT_VIDEO)) return false;
            this->main_width  = width;
            this->main_height = height;
            SDL_CreateWindowAndRenderer("Chico de Juego", width*scale, height*scale, 0,
                                        &this->main_window, &this->main_renderer);
            if (!this->main_window || !this->main_renderer) return false;
            SDL_SetWindowTitle(this->main_window, "Chico de Juego");
            this->main_texture = SDL_CreateTexture(this->main_renderer,
                SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
            SDL_SetRenderVSync(this->main_renderer, 1);
            SDL_SetTextureScaleMode(this->main_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderLogicalPresentation(this->main_renderer, width, height,
                SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
            return true;
        }

        void shutdown() override {
            for (int i = 0; i < NUM_DEBUG_WINDOWS; i++)
                close_aux((DebugWindowType)i);
            if (this->main_texture)  SDL_DestroyTexture(this->main_texture);
            if (this->main_renderer) SDL_DestroyRenderer(this->main_renderer);
            if (this->main_window)   SDL_DestroyWindow(this->main_window);
            this->main_texture  = nullptr;
            this->main_renderer = nullptr;
            this->main_window   = nullptr;
        }

        void present(const u32* fb) override {
            if (!this->main_renderer) return;
            SDL_UpdateTexture(this->main_texture, NULL, fb, this->main_width * sizeof(u32));
            SDL_RenderClear(this->main_renderer);
            SDL_RenderTexture(this->main_renderer, this->main_texture, NULL, NULL);
            SDL_RenderPresent(this->main_renderer);
        }

        void set_title(const char* title) override {
            if (this->main_window) SDL_SetWindowTitle(this->main_window, title);
        }

        void open_aux(DebugWindowType w, int width, int height, int scale, const char* title) override {
            int i = (int)w;
            Aux& a = aux[i];
            if (a.open) return;

            SDL_CreateWindowAndRenderer(title, width * scale, height * scale, 0,
                                        &a.window, &a.renderer);
            if (!a.window || !a.renderer) return;
            a.texture = SDL_CreateTexture(a.renderer,
                SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
            SDL_SetTextureScaleMode(a.texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderLogicalPresentation(a.renderer, width, height,
                SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

            // They stack to the right of the main window, each one below the
            // lower-indexed ones that are already open.
            int main_x, main_y, main_w, main_h;
            SDL_GetWindowPosition(this->main_window, &main_x, &main_y);
            SDL_GetWindowSize(this->main_window, &main_w, &main_h);
            int y = main_y;
            for (int j = 0; j < i; j++)
                if (aux[j].open)
                    y += aux[j].height * aux[j].scale;
            SDL_SetWindowPosition(a.window, main_x + main_w, y);

            a.height = height;
            a.scale  = scale;
            a.open   = true;
        }

        void close_aux(DebugWindowType w) override {
            Aux& a = aux[(int)w];
            if (!a.open) return;
            SDL_DestroyTexture(a.texture);
            SDL_DestroyRenderer(a.renderer);
            SDL_DestroyWindow(a.window);
            a.texture  = nullptr;
            a.renderer = nullptr;
            a.window   = nullptr;
            a.open     = false;
        }

        void present_aux(DebugWindowType w, const u32* fb, int width) override {
            Aux& a = aux[(int)w];
            if (!a.open) return;
            SDL_UpdateTexture(a.texture, NULL, fb, width * sizeof(u32));
            SDL_RenderClear(a.renderer);
            SDL_RenderTexture(a.renderer, a.texture, NULL, NULL);
            SDL_RenderPresent(a.renderer);
        }

        // Needed by Sdl_system to match close events with their window.
        SDL_WindowID main_window_id() const {
            return this->main_window ? SDL_GetWindowID(this->main_window) : 0;
        }
        SDL_WindowID aux_window_id(int i) const {
            return aux[i].open ? SDL_GetWindowID(aux[i].window) : 0;
        }
};

// Single instance of the video backend. Sdl_system queries it to find out which
// window each close event belongs to.
static Sdl_video g_sdl_video;

IVideo* create_video(){
    return &g_sdl_video;
}

SDL_WindowID sdl_main_window_id(){
    return g_sdl_video.main_window_id();
}
SDL_WindowID sdl_aux_window_id(int i){
    return g_sdl_video.aux_window_id(i);
}
