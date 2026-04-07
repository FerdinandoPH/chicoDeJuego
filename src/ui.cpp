#include "ui.h"
#include "debugger.h"

Ui::Ui(Memory& mem, Controller& controller, int scale) :  mem(mem), controller(controller), scale(scale), video_buffer_mutex() {

}
void Ui::init(){
    // SDL_CreateWindowAndRenderer("Tilemap Debugger", 16*8*scale, 24*8*scale, 0, &this->tile_debug_window, &this->tile_debug_renderer);
    // this->tile_debug_surface = SDL_CreateSurface(16*8*scale + 16*scale, 24*8*scale + 24*scale, SDL_PIXELFORMAT_ARGB8888);
    // this->tile_debug_texture = SDL_CreateTexture(this->tile_debug_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 16*8*scale + 16*scale, 24*8*scale + 24*scale);
    // SDL_SetWindowPosition(this->tile_debug_window, 1300, 150);

    this->video_buffer = new u32[XRES * YRES];
    SDL_CreateWindowAndRenderer("Chico de Juego", XRES*scale, YRES*scale, 0, &this->main_window, &this->main_renderer);
    SDL_SetWindowTitle(this->main_window, "Chico de Juego");
    this->main_texture = SDL_CreateTexture(this->main_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, XRES, YRES);
    SDL_SetRenderVSync(this->main_renderer, 1);
    // Pixel-perfect rendering: avoid linear filtering artifacts when scaling.
    SDL_SetTextureScaleMode(this->main_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(this->main_renderer, XRES, YRES, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}
void Ui::set_debugger(Debugger* dbg){
    this->dbg = dbg;
}
bool Ui::handle_events(){
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0){
        switch (event.type){
            case SDL_EVENT_QUIT:
                // SDL_DestroyWindow(this->tile_debug_window);
                // SDL_DestroyRenderer(this->tile_debug_renderer);
                // SDL_DestroyTexture(this->tile_debug_texture);
                // SDL_DestroySurface(this->tile_debug_surface);
                SDL_DestroyWindow(this->main_window);
                SDL_DestroyRenderer(this->main_renderer);
                SDL_DestroyTexture(this->main_texture);
                //SDL_Quit();
                return false;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                Uint32 windowID = event.window.windowID;
                Uint32 main_windowID = SDL_GetWindowID(this->main_window);
                //Uint32 debug_windowID = SDL_GetWindowID(this->tile_debug_window);

                if (windowID == main_windowID){// || windowID == debug_windowID){
                    // SDL_DestroyWindow(this->tile_debug_window);
                    // SDL_DestroyRenderer(this->tile_debug_renderer);
                    // SDL_DestroyTexture(this->tile_debug_texture);
                    // SDL_DestroySurface(this->tile_debug_surface);
                    SDL_DestroyWindow(this->main_window);
                    SDL_DestroyRenderer(this->main_renderer);
                    SDL_DestroyTexture(this->main_texture);
                    // SDL_Quit();
                    // exit(0);
                    return false;
                }
                break;
            }
            case SDL_EVENT_KEY_DOWN:
                switch (event.key.scancode){
                    case SDL_SCANCODE_ESCAPE:
                        this->dbg->dbg_level = FULL_DBG;
                        break;
                    default:
                        if (!event.key.repeat){
                            //printf("Key down: %s\n", SDL_GetScancodeName(event.key.scancode));
                            this->controller.enqueue_event(event.key.scancode, Controller_event_type::KEY_DOWN);
                        }

                        break;
                }
                break;
            case SDL_EVENT_KEY_UP:
                if (!event.key.repeat){
                    //printf("Key up: %s\n", SDL_GetScancodeName(event.key.scancode));
                    this->controller.enqueue_event(event.key.scancode, Controller_event_type::KEY_UP);
                }
                break;
            default:
                break;
        }
    }
    return true;
}
bool Ui::update() {
    if (this->change_requested){
        this->screens_on = !this->screens_on;
        this->change_requested = false;
        if (this->screens_on){
            //SDL_ShowWindow(this->tile_debug_window);
            SDL_ShowWindow(this->main_window);
        } else {
            //SDL_HideWindow(this->tile_debug_window);
            SDL_HideWindow(this->main_window);
        }
    }
    if(this->screens_on){
        if(!this->handle_events())
            return false;
        this->main_screen_update();
        //this->tile_dbg_update();
    }
    return true;
}
void Ui::tile_dbg_update(){
    SDL_Rect bg_rect = {0, 0, this->tile_debug_surface->w, this->tile_debug_surface->h};
    SDL_FillSurfaceRect(this->tile_debug_surface, &bg_rect, 0x000000);
    //384 tiles, 24 x 16
    for (int y = 0; y < 24; y++){
        for (int x = 0; x < 16; x++){
            tile_display(y*16 + x, x*8*scale + (x*scale), y*8*scale + (y*scale)); //tile addr start, x (adding space), y (adding space)
        }
    }
    SDL_UpdateTexture(this->tile_debug_texture, NULL, this->tile_debug_surface->pixels, this->tile_debug_surface->pitch);
    SDL_RenderClear(this->tile_debug_renderer);
    SDL_RenderTexture(this->tile_debug_renderer, this->tile_debug_texture, NULL, NULL);
    SDL_RenderPresent(this->tile_debug_renderer);
}
void Ui::tile_display(u16 tile, int x, int y){
    SDL_Rect rect;
    for (int line = 0; line < 16; line+=2){
        u8 msb = this->mem.readX(0x8000 + tile*16 + line);
        u8 lsb = this->mem.readX(0x8000 + tile*16 + line + 1);
        for (int pixel = 0; pixel < 8; pixel++){
            u8 color = ((msb >> (7-pixel)) & 0x1) << 1 | ((lsb >> (7-pixel)) & 0x1);
            rect.x = x + pixel*scale;
            rect.y = y + (line/2)*scale;
            rect.w = rect.h = scale;
            SDL_FillSurfaceRect(this->tile_debug_surface, &rect, gb_palette[color]);
        }
    }
}
void Ui::write_pixel(int x, int y, u32 color){
    if (x < 0 || x >= XRES || y < 0 || y >= YRES) return;
    std::scoped_lock<std::mutex> lock(video_buffer_mutex);
    this->video_buffer[y * XRES + x] = color;
}
void Ui::main_screen_update(){
    video_buffer_mutex.lock();
    SDL_UpdateTexture(this->main_texture, NULL, this->video_buffer, XRES * sizeof(u32));
    video_buffer_mutex.unlock();
    SDL_RenderClear(this->main_renderer);
    SDL_RenderTexture(this->main_renderer, this->main_texture, NULL, NULL);
    SDL_RenderPresent(this->main_renderer);
}