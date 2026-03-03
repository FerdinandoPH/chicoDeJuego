#include <ppu.h>

Pixel_Fetcher::Pixel_Fetcher(Memory& mem) : mem(mem) {
    this->state = Pixel_fetcher_state::READ_TILE;
    this->tile_type = Tile_type::BG;
}
void Pixel_Fetcher::set_fifo(Pixel_FIFO* fifo){
    this->fifo = fifo;
}
Pixel_FIFO::Pixel_FIFO(Memory& mem, Ui* ui, Sprite (&line_oam)[10], int& sprites_in_line, Pixel_Fetcher* fetcher) : mem(mem), ui(ui), line_oam(line_oam), sprites_in_line(sprites_in_line), fetcher(fetcher) {
    this->lx = 0;
}

// Fetcher
void Pixel_Fetcher::new_line(){
    f_ly++;
    this->state = Pixel_fetcher_state::READ_TILE;
    this->tile_type = Tile_type::BG;
}
void Pixel_Fetcher::new_frame(){
    f_ly = 0;
    this->state = Pixel_fetcher_state::READ_TILE;
    this->tile_type = Tile_type::BG;
}
void Pixel_Fetcher::tick(){
    switch(state){
        case Pixel_fetcher_state::READ_TILE:
            break;
    }
}
u8 Pixel_FIFO::get_lx(){
    return this->lx;
}
u8 Pixel_FIFO::get_win_ly(){
    return this->win_ly;
}
void Pixel_FIFO::new_frame(){
    this->lx = 0;
    this->ly = 0;
    this->win_ly = 0;
    this->waiting_for_sprite = false;
    this->pixels.clear();
    this->obj_pixels.clear();
    this->sprites_in_pixel.clear();
    this->fetcher->new_frame();
    this->wy_cond = false;
    this->wx_cond = false;
    this->window_active = false;
}
void Pixel_FIFO::new_line(){
    this->lx = 0;
    this->ly++;
    this->waiting_for_sprite = false;
    this->pixels.clear();
    this->obj_pixels.clear();
    this->sprites_in_pixel.clear();
    if(!this->wy_cond && this->mem.readX(WY_ADDR) == this->mem.readX(LY_ADDR)){
        this->wy_cond = true;
    }
    this->wx_cond = false;
    this->window_active = false;
    this->pixels_to_discard = this->mem.readX(SCX_ADDR) % 8;
}
u32 Pixel_FIFO::get_final_color(Pixel pixel){
    u16 palette_addr;
    switch(pixel.palette){
        case Palette::OBJ0:
            palette_addr = OBP0_ADDR;
            break;
        case Palette::OBJ1:
            palette_addr = OBP1_ADDR;
            break;
        default: // BG/WIN
            palette_addr = BGP_ADDR;
            break;
    }
    u8 palette_data = mem.readX(palette_addr);
    u8 color_id = (palette_data >> (pixel.color_index * 2)) & 0x3;
    return gb_palette[color_id];
}
void Pixel_FIFO::tick(){
    if(waiting_for_sprite) return;
    if(!window_active && wy_cond && wx_cond && (mem.readX(LCDC_ADDR) & 0x20)){
        window_active = true;
        pixels.clear();
        fetcher->change_to_win();
        return;
    }
    if(this->pixels.empty()) return;
    if(pixels_to_discard>0){
        pixels_to_discard--;
        pixels.pop_front();
        return;
    }
    if(!sprites_in_pixel.empty()){
        Sprite spr = sprites_in_pixel.front();
        sprites_in_pixel.pop_front();
        fetcher->fetch_sprite(spr);
        waiting_for_sprite = true;
        return;
    }
    if(!sprites_in_pixel_done){
        for(int i = 0; i < sprites_in_line; i++){
            Sprite spr = line_oam[i];
            if(spr.x_pos == lx + 8){
                sprites_in_pixel.push_back(spr);
            }
        }
        sprites_in_pixel_done = true;
        if(!sprites_in_pixel.empty()){
            Sprite spr = sprites_in_pixel.front();
            sprites_in_pixel.pop_front();
            fetcher->fetch_sprite(spr);
            waiting_for_sprite = true;
            return;
        }
    }

    Pixel pixel = this->pixels.front();
    this->pixels.pop_front();
    if(pixel.tile_type==Tile_type::WINDOW)
        win_ly++;
    if (!obj_pixels.empty()){
        Pixel obj_pixel = obj_pixels.front();
        obj_pixels.pop_front();
        if(obj_pixel.color_index != 0 && (pixel.color_index == 0 || !obj_pixel.spr_priority)){ // If the obj pixel is not transparent, and the BG/WIN is either 0 or the sprite has priority(Beware: the sprite priority bit set to 1 actually means that BG/WIN have priority)
            pixel = obj_pixel;
        }
    }
    if(lx>=8){
        this->ui->write_pixel(this->lx-8, this->ly, get_final_color(pixel));
    }
    if(lx == this->mem.readX(WX_ADDR)){
        this->wx_cond = true;
    }
    lx++;
}