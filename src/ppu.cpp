#include <ppu.h>

std::map<Ppu_mode, std::string> ppu_mode_names = {
    {Ppu_mode::OAM, "OAM"}, {Ppu_mode::TRANSFER, "TRANSFER"}, {Ppu_mode::VBLANK, "VBLANK"}, {Ppu_mode::HBLANK, "HBLANK"}
};
Ppu::Ppu(Memory& mem, int scale) : mem(mem), scale(scale) {
    this->ppu_mode = Ppu_mode::VBLANK;
}

void Ppu::tick(){
    this->line_ticks++;
    switch(this->ppu_mode){
        case Ppu_mode::OAM:
            if (this->line_ticks >= 80){
                this->change_mode(Ppu_mode::TRANSFER);
            }
            break;
        case Ppu_mode::TRANSFER:
            if (this->line_ticks >= 252){
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