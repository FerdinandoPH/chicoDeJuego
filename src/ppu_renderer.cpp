#include <ppu.h>

Pixel_Fetcher::Pixel_Fetcher(Memory& mem, Sprite (&line_oam)[10], int& sprites_in_line) : mem(mem), line_oam(line_oam), sprites_in_line(sprites_in_line) {
    this->state = Pixel_fetcher_state::READ_TILE;
    this->tile_type = Tile_type::BG;
}
void Pixel_Fetcher::set_fifo(Pixel_FIFO* fifo){
    this->fifo = fifo;
}
Pixel_FIFO::Pixel_FIFO(Memory& mem, Sprite (&line_oam)[10], int& sprites_in_line, Pixel_Fetcher* fetcher) : mem(mem), line_oam(line_oam), sprites_in_line(sprites_in_line), fetcher(fetcher) {
    this->dx = 0;
}

// Fetcher
void Pixel_Fetcher::new_line(){
    dx = 0;
    dy++;
    this->state = Pixel_fetcher_state::READ_TILE;
    this->tile_type = Tile_type::BG;
}
void Pixel_Fetcher::new_frame(){
    dx = dy = 0;
    this->state = Pixel_fetcher_state::READ_TILE;
    this->tile_type = Tile_type::BG;
}
void Pixel_Fetcher::fetch_sprite(Sprite spr, int line){
    this->spr = spr;
    this->spr_line = line;
    this->tile_type_bak = this->tile_type;
    this->tile_type = Tile_type::SPRITE;
    this->state = Pixel_fetcher_state::READ_TILE;
}

void Pixel_Fetcher::tick(){
    u8 wx = mem.readX(WX_ADDR);
    u8 wy = mem.readX(WY_ADDR);
    wx = wx >= 7 ? wx - 7 : 0;
    win_dx = dx >= wx ? dx - wx : 0;
    win_dy = dy >= wy ? dy - wy : 0;
    switch(this->state){
        case Pixel_fetcher_state::READ_TILE:
            switch(this->tile_type){
                case Tile_type::SPRITE:
                    u16 final_index = spr.tile_index;
                    if (mem.readX(LCDC_ADDR) & 0x4){ // If sprites are 8x16
                        final_index &= 0xFE;
                        if (spr_line > 8 ^ spr.y_flip){ // If the sprite is in the second tile, or if it's flipped and the line is in the first tile
                            final_index++;
                        }
                    }
                    tile_addr = 0x8000 + final_index * 16;
                    break;
                case Tile_type::BG:
                    u16 map_addr;
                    map_addr = mem.readX(LCDC_ADDR) & 8 ? 0x9C00 : 0x9800;
                    map_addr += (dx + mem.readX(SCX_ADDR)) / 8 + ((mem.readX(SCY_ADDR) + dy) / 8) * 32;
                    tile_addr = mem.readX(map_addr) * 16 + ((mem.readX(LCDC_ADDR) & 0x10) ? 0x8000 : 0x8800);
                    break;
                case Tile_type::WINDOW:
                    u16 map_addr;
                    map_addr = mem.readX(LCDC_ADDR) & 0x40 ? 0x9C00 : 0x9800;
                    map_addr += win_dx / 8 + (win_dy / 8) * 32;
                    tile_addr = mem.readX(map_addr) * 16 + ((mem.readX(LCDC_ADDR) & 0x10) ? 0x8000 : 0x8800);
                    break;
            }
            this->state = Pixel_fetcher_state::READ_DATA_LO;
            break;
        case Pixel_fetcher_state::READ_DATA_LO:
            tile_lo = mem.readX(tile_addr + get_line_to_read() * 2);
            this->state = Pixel_fetcher_state::READ_DATA_HI;
            break;
        case Pixel_fetcher_state::READ_DATA_HI:
            tile_hi = mem.readX(tile_addr + get_line_to_read() * 2 + 1);
            assemble_pixels();
            this->state = Pixel_fetcher_state::PUSH;
            break;
        case Pixel_fetcher_state::PUSH:
            if(this->tile_type == Tile_type::SPRITE){
                fifo->mix_sprite(pixels);
                this->tile_type = this->tile_type_bak;
            }
            else if(fifo->size() <= 8){
                fifo->push(pixels);
                dx += 8;
                this->state = Pixel_fetcher_state::READ_TILE;
            }
            break;
    }
}
u8 Pixel_Fetcher::get_line_to_read(){
    u8 line_to_read = 0;
    switch(tile_type){
        case Tile_type::SPRITE:
            line_to_read += spr_line % 8;
            if (spr.y_flip) line_to_read = 7 - line_to_read;
            break;
        case Tile_type::BG:
            line_to_read += (dy + mem.readX(SCY_ADDR)) % 8;
            break;
        case Tile_type::WINDOW:
            line_to_read += win_dy % 8;
            break;
    }
    return line_to_read;
}
void Pixel_Fetcher::assemble_pixels(){
    bool is_sprite = tile_type == Tile_type::SPRITE;
    bool is_x_flipped_sprite = is_sprite && spr.x_flip;
    for (int i = 0; i < 8; i++){
        u8 color_index = ((tile_hi >> (7 - i)) & 1) << 1 | ((tile_lo >> (7 - i)) & 1);
        Pixel pixel = {color_index, Palette::BG, tile_type};
        if (is_sprite){
            pixel.palette = spr.palette ? Palette::OBJ1 : Palette::OBJ0;
        }
        pixels[is_x_flipped_sprite ? 7 - i : i] = pixel;
    }
}