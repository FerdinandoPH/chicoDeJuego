#include "apu_channels.h"
#include "memory.h"
#include "hw_reg_def.h"
void Pulse_1_channel::set_sweep_pace(u8 value, bool internal){
    if (internal) internal_regs.sweep_pace = value;
    else{
        external_regs.sweep_pace = value;
        if (value == 0 || internal_regs.sweep_pace == 0)
            internal_regs.sweep_pace = value;
    }
}
void Channel::internal_reset(){
    length_limit = 0;
    internal_regs.volume = 0;
    frequency = 0;
    external_regs = internal_regs;
    length_timer = 0;
    volume_timer = 0;
    playing = false;
    dac_enabled = false;
    length_enabled = false;
}

void Pulse_channel::internal_reset(){
    Channel::internal_reset();
    internal_regs.duty_cycle = 0;
    internal_regs.envelope_dir = 0;
    internal_regs.envelope_pace = 0;
    external_regs = internal_regs;
    duty_idx = 0;
}
void Pulse_1_channel::internal_reset(){
    Pulse_channel::internal_reset();
    internal_regs.sweep_pace = 0;
    sweep_direction = 0;
    sweep_step = 0;
    sweep_iter_counter = 0;
    shadow_frequency = 0;
    external_regs = internal_regs;
    negative_sweep_occurred = false;
}
void Wave_channel::internal_reset(){
    Channel::internal_reset();
    for (int i = 0; i < 16; i++){
        wave_pattern[i] = 0;
    }
    wave_idx = 0;
}
void Noise_channel::internal_reset(){
    Channel::internal_reset();
    internal_regs.envelope_dir = 0;
    internal_regs.envelope_pace = 0;
    external_regs = internal_regs;
    lfsr = 0;
    clock_divider = 0;
    clock_shift = 0;
    width_mode = Noise_width::M15;
}
void Channel::trigger(){
    volume_timer = 0;
    if (length_timer >= length_limit) length_timer = 0;
    internal_regs.volume = external_regs.volume;
    if(dac_enabled) playing = true;
}
void Pulse_channel::trigger(){
    Channel::trigger();
    internal_regs.duty_cycle = external_regs.duty_cycle;
    internal_regs.envelope_dir = external_regs.envelope_dir;
    internal_regs.envelope_pace = external_regs.envelope_pace;
    frequency_timer = (2048 - frequency) * 4;
}
void Pulse_1_channel::trigger(){
    Pulse_channel::trigger();
    internal_regs.sweep_pace = external_regs.sweep_pace;
    shadow_frequency = frequency;
    sweep_iter_counter = 0;
    negative_sweep_occurred = false;
    if (sweep_step != 0){
        negative_sweep_occurred = sweep_direction;
        u16 potential_new_freq = (shadow_frequency == 0)? shadow_frequency : shadow_frequency + (sweep_direction ? -1 : 1) * (shadow_frequency >> sweep_step);
        if (!sweep_direction && potential_new_freq > 0x7FF){
            playing = false;
        }
    }
    
}
void Wave_channel::trigger(){
    Channel::trigger();
    wave_idx = 0;
    frequency_timer = (2048 - frequency) * 2;
}
void Noise_channel::trigger(){
    Channel::trigger();
    lfsr = 0;
    internal_regs.envelope_dir = external_regs.envelope_dir;
    internal_regs.envelope_pace = external_regs.envelope_pace;
    frequency_timer = (clock_divider == 0 ? 8 : clock_divider * 16) << clock_shift;
}


void Pulse_channel::tick(){
    if(!playing) return;
    frequency_timer--;
    if(frequency_timer == 0){
        frequency_timer = (2048 - frequency) * 4;
        duty_idx = (duty_idx + 1) % 8;
    }
}
void Pulse_1_channel::tick(){
    Pulse_channel::tick();
}
void Wave_channel::tick(){
    if(!playing) return;
    frequency_timer--;
    if(frequency_timer == 0){
        wave_idx = (wave_idx + 1) % 32;
        frequency_timer = (2048 - frequency) * 2;
    }
}
void Noise_channel::tick(){
    if(!playing) return;
    frequency_timer--;
    if(frequency_timer == 0){
        frequency_timer = (clock_divider == 0 ? 8 : clock_divider * 16) << clock_shift;
        u16 new_bit = ((lfsr & 1) == ((lfsr >> 1) & 1)) ? 1 : 0;
        lfsr = (new_bit << 15) | (lfsr & 0x7FFF);
        if (width_mode == Noise_width::M7)
            lfsr = (new_bit << 7) | (lfsr & 0xFF7F);
        lfsr >>= 1;
    }
}

void Channel::frame_seq_tick(u8 frame_step){
    if((frame_step % 2) != 0) return;
    if(length_enabled){
        length_timer++;
        if (length_timer >= length_limit){
            playing = false;
            length_timer = 0;
        }
    }
}
void Pulse_channel::frame_seq_tick(u8 frame_step){
    Channel::frame_seq_tick(frame_step);
    if (frame_step == 7){
        if(internal_regs.envelope_pace != 0){
            volume_timer++;
            if (volume_timer >= internal_regs.envelope_pace){
                volume_timer = 0;
                if (internal_regs.envelope_dir && internal_regs.volume < 15)
                    internal_regs.volume++;
                else if (!internal_regs.envelope_dir && internal_regs.volume > 0){
                    internal_regs.volume--;
                    //if (internal_regs.volume == 0) playing = false;
                }
            }
        }
    }
}
void Pulse_1_channel::frame_seq_tick(u8 frame_step){
    Pulse_channel::frame_seq_tick(frame_step);
    if ((frame_step == 2 || frame_step == 6) && internal_regs.sweep_pace != 0){
        sweep_iter_counter++;
        if (sweep_iter_counter >= internal_regs.sweep_pace){
            internal_regs.sweep_pace = external_regs.sweep_pace;
            sweep_iter_counter = 0;
            negative_sweep_occurred = sweep_direction;
            u16 new_frequency = shadow_frequency == 0 ? shadow_frequency : shadow_frequency + (sweep_direction ? -1 : 1) * (shadow_frequency >> sweep_step);
            if (!sweep_direction && new_frequency > 0x7FF){
                playing = false;
            }else if (sweep_step != 0){
                frequency = new_frequency;
                shadow_frequency = new_frequency;
                mem.writeX(NR13_ADDR, (u8)(new_frequency & 0xFF));
                mem.writeX(NR14_ADDR, (u8)((mem.readX(NR14_ADDR) & 0xF8) | ((new_frequency >> 8) & 0x7)));
                u16 potential_new_freq = (shadow_frequency == 0)? shadow_frequency : shadow_frequency + (sweep_direction ? -1 : 1) * (shadow_frequency >> sweep_step);
                if (!sweep_direction && potential_new_freq > 0x7FF){
                    playing = false;
                }
            }
        }
    }
}
void Wave_channel::frame_seq_tick(u8 frame_step){
    Channel::frame_seq_tick(frame_step);
}
void Noise_channel::frame_seq_tick(u8 frame_step){
    Channel::frame_seq_tick(frame_step);
    if (frame_step == 7){
        if(internal_regs.envelope_pace != 0){
            volume_timer++;
            if (volume_timer >= internal_regs.envelope_pace){
                volume_timer = 0;
                if (internal_regs.envelope_dir && internal_regs.volume < 15)
                    internal_regs.volume++;
                else if (!internal_regs.envelope_dir && internal_regs.volume > 0){
                    internal_regs.volume--;
                    //if (internal_regs.volume == 0) playing = false;
                }
            }
        }
    }
}

double Pulse_channel::generate_sample(){
    u8 base = duty_table[internal_regs.duty_cycle][duty_idx] * internal_regs.volume;
    return (base - 7.5f) / 7.5f; //normalize to [-1, 1]
}
double Wave_channel::generate_sample(){
    if (internal_regs.volume == 0) return 0.0;
    u8 base_byte = wave_pattern[wave_idx / 2]; //each byte has 2 samples
    u8 base = (wave_idx % 2 == 0) ? (base_byte >> 4) : (base_byte & 0xF); //high nibble for even idx, low nibble for odd idx
    base >>= internal_regs.volume - 1;
    return (base - 7.5f) / 7.5f; //normalize to [-1, 1]
}
double Noise_channel::generate_sample(){
    u8 base = ((lfsr & 1) == 0 ? 1 : 0) * internal_regs.volume;
    return (base - 7.5f) / 7.5f; //normalize to [-1, 1]
}

// --- Save state ---

void Channel::save_base(Channel_ss& ss) const {
    ss.length_limit    = length_limit;
    ss.length_timer    = length_timer;
    ss.volume_timer    = volume_timer;
    ss.frequency       = frequency;
    ss.frequency_timer = frequency_timer;
    ss.playing         = playing;
    ss.dac_enabled     = dac_enabled;
    ss.length_enabled  = length_enabled;
    ss.ext_volume      = external_regs.volume;
    ss.int_volume      = internal_regs.volume;
}
void Channel::load_base(const Channel_ss& ss){
    length_limit       = ss.length_limit;
    length_timer       = ss.length_timer;
    volume_timer       = ss.volume_timer;
    frequency          = ss.frequency;
    frequency_timer    = ss.frequency_timer;
    playing            = ss.playing;
    dac_enabled        = ss.dac_enabled;
    length_enabled     = ss.length_enabled;
    external_regs.volume = ss.ext_volume;
    internal_regs.volume = ss.int_volume;
}

Pulse_channel_ss Pulse_channel::save_state() const {
    Pulse_channel_ss ss;
    save_base(ss.base);
    ss.duty_idx          = duty_idx;
    ss.ext_duty_cycle    = external_regs.duty_cycle;
    ss.int_duty_cycle    = internal_regs.duty_cycle;
    ss.ext_envelope_dir  = external_regs.envelope_dir;
    ss.int_envelope_dir  = internal_regs.envelope_dir;
    ss.ext_envelope_pace = external_regs.envelope_pace;
    ss.int_envelope_pace = internal_regs.envelope_pace;
    return ss;
}
void Pulse_channel::load_state(const Pulse_channel_ss& ss){
    load_base(ss.base);
    duty_idx                   = ss.duty_idx;
    external_regs.duty_cycle   = ss.ext_duty_cycle;
    internal_regs.duty_cycle   = ss.int_duty_cycle;
    external_regs.envelope_dir = ss.ext_envelope_dir;
    internal_regs.envelope_dir = ss.int_envelope_dir;
    external_regs.envelope_pace = ss.ext_envelope_pace;
    internal_regs.envelope_pace = ss.int_envelope_pace;
}

Pulse_1_channel_ss Pulse_1_channel::save_state() const {
    Pulse_1_channel_ss ss;
    ss.base = Pulse_channel::save_state();
    ss.ext_sweep_pace          = external_regs.sweep_pace;
    ss.int_sweep_pace          = internal_regs.sweep_pace;
    ss.sweep_direction         = sweep_direction;
    ss.sweep_step              = sweep_step;
    ss.sweep_iter_counter      = sweep_iter_counter;
    ss.shadow_frequency        = shadow_frequency;
    ss.negative_sweep_occurred = negative_sweep_occurred;
    return ss;
}
void Pulse_1_channel::load_state(const Pulse_1_channel_ss& ss){
    Pulse_channel::load_state(ss.base);
    external_regs.sweep_pace  = ss.ext_sweep_pace;
    internal_regs.sweep_pace  = ss.int_sweep_pace;
    sweep_direction           = ss.sweep_direction;
    sweep_step                = ss.sweep_step;
    sweep_iter_counter        = ss.sweep_iter_counter;
    shadow_frequency          = ss.shadow_frequency;
    negative_sweep_occurred   = ss.negative_sweep_occurred;
}

Wave_channel_ss Wave_channel::save_state() const {
    Wave_channel_ss ss;
    save_base(ss.base);
    for (int i = 0; i < 16; i++) ss.wave_pattern[i] = wave_pattern[i];
    ss.wave_idx = wave_idx;
    return ss;
}
void Wave_channel::load_state(const Wave_channel_ss& ss){
    load_base(ss.base);
    for (int i = 0; i < 16; i++) wave_pattern[i] = ss.wave_pattern[i];
    wave_idx = ss.wave_idx;
}

Noise_channel_ss Noise_channel::save_state() const {
    Noise_channel_ss ss;
    save_base(ss.base);
    ss.ext_envelope_dir  = external_regs.envelope_dir;
    ss.int_envelope_dir  = internal_regs.envelope_dir;
    ss.ext_envelope_pace = external_regs.envelope_pace;
    ss.int_envelope_pace = internal_regs.envelope_pace;
    ss.lfsr              = lfsr;
    ss.clock_divider     = clock_divider;
    ss.clock_shift       = clock_shift;
    ss.width_mode        = width_mode;
    return ss;
}
void Noise_channel::load_state(const Noise_channel_ss& ss){
    load_base(ss.base);
    external_regs.envelope_dir  = ss.ext_envelope_dir;
    internal_regs.envelope_dir  = ss.int_envelope_dir;
    external_regs.envelope_pace = ss.ext_envelope_pace;
    internal_regs.envelope_pace = ss.int_envelope_pace;
    lfsr          = ss.lfsr;
    clock_divider = ss.clock_divider;
    clock_shift   = ss.clock_shift;
    width_mode    = ss.width_mode;
}