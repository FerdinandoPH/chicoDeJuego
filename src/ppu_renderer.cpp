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
void Pixel_Fetcher::set_f_win_ly(u8 new_value){
    this->f_win_ly = new_value;
}
void Pixel_Fetcher::new_line(){
    f_ly++;
    //win ly is set by FIFO
    f_lx = f_win_lx = 0;
    f_fine_scx = mem.readX(SCX_ADDR) % 8;
    this->state = Pixel_fetcher_state::READ_TILE;
    this->tile_type = Tile_type::BG;
}
void Pixel_Fetcher::new_frame(){
    f_ly = 0xFF;
    this->state = Pixel_fetcher_state::READ_TILE;
    this->tile_type = Tile_type::BG;
}
void Pixel_Fetcher::fetch_sprite(Sprite spr){
    this->spr = spr;
    this->spr_line = f_ly - spr.y_pos + 16;
    this->tile_type_bak = this->tile_type;
    this->tile_type = Tile_type::SPRITE;
    this->state = Pixel_fetcher_state::READ_TILE;
}
void Pixel_Fetcher::change_to_win(){
    this->tile_type = Tile_type::WINDOW;
    this->state = Pixel_fetcher_state::READ_TILE;
}
u8 Pixel_Fetcher::get_line_to_read(){
    u8 line_to_read = 0;
    switch(tile_type){
        case Tile_type::SPRITE:
            line_to_read += spr_line % 8;
            if (spr.y_flip) line_to_read = 7 - line_to_read;
            break;
        case Tile_type::BG:
            line_to_read += (f_ly + f_scy) % 8;
            break;
        case Tile_type::WINDOW:
            line_to_read += f_win_ly % 8;
            break;
    }
    return line_to_read;
}
void Pixel_Fetcher::assemble_pixels(){
    bool is_sprite = tile_type == Tile_type::SPRITE;
    bool is_x_flipped_sprite = is_sprite && spr.x_flip;
    for (int i = 0; i < 8; i++){
        u8 color_index = ((tile_hi >> (7 - i)) & 1) << 1 | ((tile_lo >> (7 - i)) & 1);
        Pixel pixel = {color_index, Palette::BG, tile_type, spr.priority};
        if (is_sprite){
            pixel.palette = spr.palette ? Palette::OBJ1 : Palette::OBJ0;
        }
        pixel_buffer[is_x_flipped_sprite ? 7 - i : i] = pixel;
    }
}
void Pixel_Fetcher::tick(){
    switch(state){
        case Pixel_fetcher_state::READ_TILE:
            switch(this->tile_type){
                case Tile_type::SPRITE:{
                    u8 final_index = spr.tile_index;
                    if (mem.readX(LCDC_ADDR) & 0x4){ // If sprites are 8x16
                        final_index &= 0xFE;
                        if ((spr_line > 8) ^ spr.y_flip){ // If the sprite is in the second tile, or if it's flipped and the line is in the first tile
                            final_index++;
                        }
                    }
                    tile_addr = 0x8000 + final_index * 16;
                    break;
                }
                case Tile_type::BG:{
                    f_scx = mem.readX(SCX_ADDR);
                    f_scy = mem.readX(SCY_ADDR);
                    u16 map_addr;
                    map_addr = mem.readX(LCDC_ADDR) & 8 ? 0x9C00 : 0x9800; //LCDC bit 3 specifies the BG tile map area
                    map_addr += ((f_lx + f_scx) % 256) / 8 + (((f_scy + f_ly) % 256) / 8) * 32;
                    bool $8800_addressing = !(mem.readX(LCDC_ADDR) & 0x10); //LCDC bit 4 specifies the BG & Window tile data area
                    u8 tile_index = mem.readX(map_addr);
                    if($8800_addressing){
                        tile_addr = 0x9000 + static_cast<i8>(tile_index) * 16;
                    }else{
                        tile_addr = 0x8000 + tile_index * 16;
                    }
                    break;
                }
                case Tile_type::WINDOW:{
                    u16 map_addr;
                    map_addr = mem.readX(LCDC_ADDR) & 0x40 ? 0x9C00 : 0x9800; //LCDC bit 6 specifies the Window tile map area
                    map_addr += (f_win_lx / 8) + (f_win_ly / 8) * 32;
                    tile_addr = mem.readX(map_addr) * 16 + ((mem.readX(LCDC_ADDR) & 0x10) ? 0x8000 : 0x8800);
                    break;
                }
            }
            this->state = Pixel_fetcher_state::READ_TILE_2;
            break;
        case Pixel_fetcher_state::READ_TILE_2:
            this->state = Pixel_fetcher_state::READ_DATA_LO;
            break;
        case Pixel_fetcher_state::READ_DATA_LO:
            tile_lo = mem.readX(tile_addr + get_line_to_read() * 2);
            this->state = Pixel_fetcher_state::READ_DATA_LO_2;
            break;
        case Pixel_fetcher_state::READ_DATA_LO_2:
            this->state = Pixel_fetcher_state::READ_DATA_HI;
            break;
        case Pixel_fetcher_state::READ_DATA_HI:
            tile_hi = mem.readX(tile_addr + get_line_to_read() * 2 + 1);
            assemble_pixels();
            this->state = Pixel_fetcher_state::READ_DATA_HI_2;
            break;
        case Pixel_fetcher_state::READ_DATA_HI_2:
            this->state = Pixel_fetcher_state::PUSH;
            break;
        case Pixel_fetcher_state::PUSH:
            if(this->tile_type == Tile_type::SPRITE){
                fifo->push_obj(pixel_buffer);
                this->tile_type = this->tile_type_bak;
                this->state = Pixel_fetcher_state::READ_TILE;
            }
            else if(fifo->is_main_fifo_empty()){
                fifo->push(pixel_buffer);
                f_lx = fifo->get_lx();
                f_win_lx = f_lx - fifo->get_triggered_wx();
                this->state = Pixel_fetcher_state::READ_TILE;
            }
            break;
    }
}

// FIFO
bool Pixel_FIFO::is_main_fifo_empty(){
    return this->pixels.empty();
}
u8 Pixel_FIFO::get_lx(){
    return this->lx;
}
u8 Pixel_FIFO::get_win_ly(){
    return this->win_ly;
}
void Pixel_FIFO::new_frame(){
    this->lx = 0;
    this->ly = 0xFF;
    this->win_ly = 0;
    this->increase_win_ly = false;
}
void Pixel_FIFO::new_line(){
    this->lx = 0;
    this->ly++;
    this->waiting_for_sprite = false;
    this->sprites_in_pixel_done = false;
    this->pixels.clear();
    this->obj_pixels.clear();
    this->sprites_in_pixel.clear();
    if(!this->wy_cond && this->mem.readX(WY_ADDR) == this->mem.readX(LY_ADDR)){
        this->wy_cond = true;
    }
    this->wx_cond = false;
    this->window_active = false;
    this->pixels_to_discard = this->mem.readX(SCX_ADDR) % 8;
    if(increase_win_ly){
        win_ly++;
        fetcher->set_f_win_ly(win_ly);
        increase_win_ly = false;
    }
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
u8 Pixel_FIFO::get_triggered_wx(){
    return triggered_wx;
}
void Pixel_FIFO::push(Pixel* pixels){
    for (int i = 0; i < 8; i++){
        this->pixels.push_back(pixels[i]);
    }
}
void Pixel_FIFO::push_obj(Pixel* pixels){
    u8 current_obj_fifo_size = obj_pixels.size();
    for (int i = 0; i < current_obj_fifo_size; i++){
        Pixel curr_pixel = obj_pixels.front();
        obj_pixels.pop_front();
        this->obj_pixels.push_back(curr_pixel.color_index == 0 && pixels[i].color_index != 0 ? pixels[i] : curr_pixel);
    }
    for(int i = current_obj_fifo_size; i < 8; i++){
        this->obj_pixels.push_back(pixels[i]);
    }
    this->waiting_for_sprite = false;
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
            if(spr.x_pos == lx){
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
        increase_win_ly = true;
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
        this->triggered_wx = lx;
    }
    lx++;
    sprites_in_pixel_done = false;
}