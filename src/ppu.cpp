#include <ppu.h>

std::unordered_map<Ppu_mode, std::string> ppu_mode_names = {
    {Ppu_mode::OAM, "OAM"}, {Ppu_mode::TRANSFER, "TRANSFER"}, {Ppu_mode::VBLANK, "VBLANK"}, {Ppu_mode::HBLANK, "HBLANK"}
};


Ppu::Ppu(Memory& mem, int scale) : mem(mem), scale(scale) {
    this->fetcher = new Pixel_Fetcher(mem);
    this->fifo = new Pixel_FIFO(mem, this->line_oam, this->sprites_in_line, this->fetcher);
    this->fetcher->set_fifo(this->fifo);
    this->ppu_mode = Ppu_mode::VBLANK;
}

void Ppu::tick(){
    this->line_ticks++;
    switch(this->ppu_mode){
        case Ppu_mode::OAM:
            if (this->line_ticks >= 80){
                this->load_oam();
                this->load_line_oam();

                this->change_mode(Ppu_mode::TRANSFER);
                this->fetcher->new_line();
                this->fifo->new_line();
            }
            break;
        case Ppu_mode::TRANSFER:
            if(line_ticks % 2) fetcher->tick();
            fifo->tick();
            if (this->fifo->get_lx() >= 160){
                this->change_mode(Ppu_mode::HBLANK);
            }
            break;
        case Ppu_mode::HBLANK:
            if (this->line_ticks >= 456){
                this->inc_ly();
                if(this->mem.readX(LY_ADDR) >= 144){
                    this->change_mode(Ppu_mode::VBLANK);
                    this->mem.writeX(0xFF0F, (u8)(this->mem.readX(0xFF0F) | 0x1)); //Vblank IF
                    if(this->mem.readX(LCD_STAT_ADDR) & 0x10){ //If stat's Vblank interrupt is enabled
                        this->mem.writeX(0xFF0F, (u8)(this->mem.readX(0xFF0F) | 0x2));
                    }
                }
                else{
                    this->change_mode(Ppu_mode::OAM);
                }
                this->line_ticks = 0;
            }
            break;
        case Ppu_mode::VBLANK:
            if (this->line_ticks >= 456){
                this->line_ticks = 0;
                this->inc_ly();
                if(this->mem.readX(LY_ADDR) >= 154){
                    this->mem.writeX(LY_ADDR, (u8)0);
                    this->change_mode(Ppu_mode::OAM);
                }
            }
            break;


    }
}
void Ppu::load_oam(){
    for (int i = 0; i < 40; i++){
        u8 data[4];
        for (int j = 0; j < 4; j++){
            data[j] = this->mem.readX(0xFE00 + i*4 + j);
        }
        this->oam[i] = Sprite(data);
    }
}
void Ppu::load_line_oam(){
    int ly = this->mem.readX(LY_ADDR);
    int obj_height = (this->mem.readX(LCDC_ADDR) & 0x4) ? 16 : 8; // If bit 2 of LCDC is enabled, sprites are 8x16
    this->sprites_in_line = 0;
    for (int i = 0; i < 40 && this->sprites_in_line < 10; i++){
        if (ly >= this->oam[i].y_pos - 16 && ly < this->oam[i].y_pos - 16 + obj_height){
            this->line_oam[this->sprites_in_line++] = this->oam[i];
        }
    }
}
void Ppu::inc_ly(){
    bool trigger_int = false;
    u8 ly = this->mem.readX(LY_ADDR);
    ly++;
    u8 lcd_stat = this->mem.readX(LCD_STAT_ADDR);
    this->mem.writeX(LY_ADDR, ly);
    if (ly == this->mem.readX(LYC_ADDR)){
        lcd_stat |= 4;
        if (lcd_stat & 0x40){
            trigger_int = true;
        }
    }
    else{
        lcd_stat &= 0b11111011;
    }
    this->mem.writeX(LCD_STAT_ADDR, lcd_stat);
    if (trigger_int){
        this->mem.writeX(0xFF0F, (u8)(this->mem.readX(0xFF0F) | 0x2)); //Stat IF
    }
}
void Ppu::change_mode(Ppu_mode mode){
    this->ppu_mode = mode;
    u8 lcd_stat = this->mem.readX(LCD_STAT_ADDR);
    lcd_stat &= 0b11111100;
    switch(mode){
        case Ppu_mode::OAM:
            lcd_stat |= 2;
            break;
        case Ppu_mode::TRANSFER:
            lcd_stat |= 3;
            break;
        case Ppu_mode::HBLANK:
            lcd_stat |= 0;
            break;
        case Ppu_mode::VBLANK:
            lcd_stat |= 1;
            break;
    }
    this->mem.writeX(LCD_STAT_ADDR, lcd_stat);
}

std::string Ppu::toString(){
    std::string str = "PPU: ";
    str += "Mode: "+ppu_mode_names[this->ppu_mode]+"  ";
    str += "LY: "+std::to_string((int)this->mem.readX(LY_ADDR))+" ";
    return str;
}