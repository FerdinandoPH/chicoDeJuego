#include <ppu.h>

std::unordered_map<Ppu_mode, std::string> ppu_mode_names = {
    {Ppu_mode::OAM, "OAM"}, {Ppu_mode::TRANSFER, "TRANSFER"}, {Ppu_mode::VBLANK, "VBLANK"}, {Ppu_mode::HBLANK, "HBLANK"}
};


Ppu::Ppu(Memory& mem, Ui* ui, int scale) : mem(mem), ui(ui), scale(scale) {
    this->fetcher = new Pixel_Fetcher(mem);
    this->fifo = new Pixel_FIFO(mem, ui, this->line_oam, this->sprites_in_line, this->fetcher);
    this->fetcher->set_fifo(this->fifo);
    this->ppu_mode = Ppu_mode::VBLANK;
    reset();
}

void Ppu::tick(){
    if(!(this->mem.readX(LCDC_ADDR) & 0x80)){
        if(!is_on) return;
        mem.set_vram_lock(false);
        mem.set_oam_lock(false);
        this->ppu_mode = Ppu_mode::VBLANK;
        this->line_ticks = 0;
        this->mem.writeX(LY_ADDR, (u8)0);
        this->mem.writeX(LCD_STAT_ADDR, static_cast<u8>(this->mem.readX(LCD_STAT_ADDR) & 0b11111100));
        is_on = false;
        return;
    }
    if(!is_on){
        this->mem.writeX(LY_ADDR, (u8)0);
        this->change_mode(Ppu_mode::OAM);
        check_lyc_at_restart();
        is_on = true;
    }
    this->line_ticks++;
    switch(this->ppu_mode){
        case Ppu_mode::OAM:
            if (this->line_ticks >= 80){
                this->load_oam();
                this->load_line_oam();
                this->change_mode(Ppu_mode::TRANSFER);
            }
            break;
        case Ppu_mode::TRANSFER:
            fetcher->tick();
            fifo->tick();
            if (this->fifo->get_lx() >= 168){
                this->change_mode(Ppu_mode::HBLANK);
            }
            break;
        case Ppu_mode::HBLANK:
            if (this->line_ticks >= 456){
                this->inc_ly();
                if(this->mem.readX(LY_ADDR) >= 144){
                    this->change_mode(Ppu_mode::VBLANK);
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
                if(this->mem.readX(LY_ADDR) >= 154 || this->startup){
                    if(this->startup){
                        this->startup = false;
                        check_lyc_at_restart();
                    }
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
    this->sprites_in_line = 0;
    if(this->mem.readX(LCDC_ADDR) & 0x2){ //If sprites are enabled (bit 1)
        u8 ly = this->mem.readX(LY_ADDR);
        int obj_height = (this->mem.readX(LCDC_ADDR) & 0x4) ? 16 : 8; // If bit 2 of LCDC is enabled, sprites are 8x16
        for (int i = 0; i < 40 && this->sprites_in_line < 10; i++){
            if (ly >= this->oam[i].y_pos - 16 && ly < this->oam[i].y_pos - 16 + obj_height){
                this->line_oam[this->sprites_in_line] = this->oam[i];
                this->sprites_in_line++;
            }
        }
    }
}
void Ppu::inc_ly(){
    bool trigger_int = false;
    u8 ly = this->mem.readX(LY_ADDR);
    ly++;
    u8 lcd_stat = this->mem.readX(LCD_STAT_ADDR);
    this->mem.writeX(LY_ADDR, ly);
    if ((ly <= 153? ly : 0) == this->mem.readX(LYC_ADDR)){
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
        this->mem.writeX(IF_ADDR, (u8)(this->mem.readX(IF_ADDR) | 0x2)); //Stat IF
    }
}
void Ppu::change_mode(Ppu_mode mode){
    Ppu_mode old_mode = this->ppu_mode;
    this->ppu_mode = mode;
    u8 lcd_stat = this->mem.readX(LCD_STAT_ADDR);
    lcd_stat &= 0b11111100;
    switch(mode){
        case Ppu_mode::OAM:
            lcd_stat |= 2;
            mem.set_oam_lock(true);
            if (old_mode != Ppu_mode::HBLANK){
                this->fetcher->new_frame();
                this->fifo->new_frame();
            }
            this->fetcher->new_line();
            this->fifo->new_line();
            break;
        case Ppu_mode::TRANSFER:
            mem.set_vram_lock(true);
            lcd_stat |= 3;
            break;
        case Ppu_mode::HBLANK:
            mem.set_vram_lock(false);
            mem.set_oam_lock(false);
            lcd_stat |= 0;
            break;
        case Ppu_mode::VBLANK:
            mem.set_vram_lock(false);
            mem.set_oam_lock(false);
            lcd_stat |= 1;
            this->mem.writeX(0xFF0F, (u8)(this->mem.readX(0xFF0F) | 0x1)); //Vblank IF
            if(this->mem.readX(LCD_STAT_ADDR) & 0x10){ //If stat's Vblank interrupt is enabled
                this->mem.writeX(0xFF0F, (u8)(this->mem.readX(0xFF0F) | 0x2));
            }
            break;
    }
    this->mem.writeX(LCD_STAT_ADDR, lcd_stat);
}

void Ppu::reset(){
    this->startup = true;
    this->ppu_mode = Ppu_mode::VBLANK;
    this->line_ticks = 456 - 64;
}

void Ppu::check_lyc_at_restart(){
    if (this->mem.readX(LYC_ADDR) == 0){
        this->mem.writeX(LCD_STAT_ADDR, (u8)(this->mem.readX(LCD_STAT_ADDR) | 0b100));
        if (this->mem.readX(LCD_STAT_ADDR) & 0x40){
            this->mem.writeX(IF_ADDR, (u8)(this->mem.readX(IF_ADDR) | 0x2));
        }
    }
}

std::string Ppu::toString(){
    std::string str = "PPU: ";
    str+= "LCD: "+std::string((this->mem.readX(LCDC_ADDR) & 0x80) ? "ON" : "OFF")+"  ";
    str += "Mode: "+ppu_mode_names[this->ppu_mode]+"  ";
    str += "LY: "+std::to_string((int)this->mem.readX(LY_ADDR))+" ";
    str += "LX: "+std::to_string(this->fifo->get_lx())+" ";
    str += "SCX: "+std::to_string((int)this->mem.readX(SCX_ADDR))+" ";
    str += "SCY: "+std::to_string((int)this->mem.readX(SCY_ADDR))+" ";
    return str;
}

Ppu_trace Ppu::get_trace(){
    Ppu_trace trace;
    trace.lcdc = this->mem.readX(LCDC_ADDR);
    trace.stat = this->mem.readX(LCD_STAT_ADDR);
    trace.scy = this->mem.readX(SCY_ADDR);
    trace.scx = this->mem.readX(SCX_ADDR);
    trace.ly = this->mem.readX(LY_ADDR);
    trace.lyc = this->mem.readX(LYC_ADDR);
    return trace;
}