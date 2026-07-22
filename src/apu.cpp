#include "apu.h"
#include "memory.h"
#include "ui.h"
#include "cpu.h"
Apu::Apu(Memory& mem, Cpu& cpu, Ui& ui) : mem(mem), cpu(cpu), ui(ui), pulse_1(mem), pulse_2(mem), wave(mem), noise(mem) {
    internal_reset();
}

void Apu::internal_reset() {
    frame_step = 0;
    last_div_bit = 0;
    pulse_1.internal_reset();
    pulse_2.internal_reset();
    wave.internal_reset();
    noise.internal_reset();
}

u8 Apu::write(u16 addr, u8 data){
    switch(addr){
        case NR52_ADDR: {
            enabled = (data & 0x80);
            if (!enabled){
                this->internal_reset();
            }
            return mem.readX(NR52_ADDR) & (enabled ? 0x8F : 0x00);
        }
        case NR51_ADDR: {
            for (int i = 0; i < 4; i++){
                stereo_left[i] = (data & (1 << i));
                stereo_right[i] = (data & (1 << (i + 4)));
            }
            return data;
        }
        case NR50_ADDR: {
            left_volume = ((data >> 4) & 0x07) + 1;
            right_volume = (data & 0x07) + 1;
            return data;
        }
        case NR10_ADDR: {
            pulse_1.set_sweep_pace((data >> 4) & 0x07);
            pulse_1.set_sweep_direction((data >> 3) & 0x01);
            pulse_1.set_sweep_step(data & 0x07);
            return data & 0x7F;
        }
        case NR11_ADDR: {
            pulse_1.set_duty_cycle((data >> 6) & 0x03);
            pulse_1.set_length_limit(64 - (data & 0x3F));
            return data;
        }
        case NR12_ADDR: {
            pulse_1.set_volume((data >> 4) & 0xF);
            pulse_1.set_envelope_dir((data >> 3) & 0x01);
            pulse_1.set_envelope_pace(data & 0x07);
            pulse_1.set_dac_enabled(data & 0xF8);
            return data;
        }
        case NR13_ADDR: {
            pulse_1.set_frequency((pulse_1.get_frequency() & 0x700) | data);
            return data;
        }
        case NR14_ADDR: {
            pulse_1.set_frequency((pulse_1.get_frequency() & 0x0FF) | ((data & 0x07) << 8));
            pulse_1.set_length_enabled(data & 0x40);
            if (data & 0x80) pulse_1.trigger();
            return data & 0xC7;
        }
        case NR21_ADDR: {
            pulse_2.set_duty_cycle((data >> 6) & 0x03);
            pulse_2.set_length_limit(64 - (data & 0x3F));
            return data;
        }
        case NR22_ADDR: {
            pulse_2.set_volume((data >> 4) & 0xF);
            pulse_2.set_envelope_dir((data >> 3) & 0x01);
            pulse_2.set_envelope_pace(data & 0x07);
            pulse_2.set_dac_enabled(data & 0xF8);
            return data;
        }
        case NR23_ADDR: {
            pulse_2.set_frequency((pulse_2.get_frequency() & 0x700) | data);
            return data;
        }
        case NR24_ADDR: {
            pulse_2.set_frequency((pulse_2.get_frequency() & 0x0FF) | ((data & 0x07) << 8));
            pulse_2.set_length_enabled(data & 0x40);
            if (data & 0x80) pulse_2.trigger();
            return data & 0xC7;
        }
        case NR30_ADDR: {
            wave.set_dac_enabled(data & 0x80);
            return data & 0x7F;
        }
        case NR31_ADDR: {
            wave.set_length_limit(256 - data);
            return data;
        }
        case NR32_ADDR: {
            wave.set_volume((data >> 5) & 0x03);
            return data & 0x9F;
        }
        case NR33_ADDR: {
            wave.set_frequency((wave.get_frequency() & 0x700) | data);
            return data;
        }
        case NR34_ADDR: {
            wave.set_frequency((wave.get_frequency() & 0x0FF) | ((data & 0x07) << 8));
            wave.set_length_enabled(data & 0x40);
            if (data & 0x80) wave.trigger();
            return data & 0xC7;
        }
        case 0xFF30 ... 0xFF3F: {
            wave.set_wave_pattern(addr - 0xFF30, data);
            return data;
        }
        case NR41_ADDR: {
            noise.set_length_limit(64 - (data & 0x3F));
            return data & 0x3F;
        }
        case NR42_ADDR: {
            noise.set_volume((data >> 4) & 0xF);
            noise.set_envelope_dir((data >> 3) & 0x01);
            noise.set_envelope_pace(data & 0x07);
            noise.set_dac_enabled(data & 0xF8);
            return data;
        }
        case NR43_ADDR: {
            noise.set_clock_divider(data & 0x07);
            noise.set_clock_shift((data >> 4) & 0x0F);
            noise.set_width_mode((data & 0x08) ? Noise_width::M7 : Noise_width::M15);
            return data;
        }
        case NR44_ADDR: {
            noise.set_length_enabled(data & 0x40);
            if (data & 0x80) noise.trigger();
            return data & 0xC0;
        }

        default:
            return data;
    }
}
double Apu::dc_block(double in, double& cap){
    double out = in - cap;
    cap = in - out * 0.999;
    return out;
}
void Apu::generate_sample(){
    //as of now, only pulse 2 is ready

    //get all samples
    double pulse_1_sample = pulse_1.is_playing() ? pulse_1.generate_sample() : 0;
    double pulse_2_sample = pulse_2.is_playing() ? pulse_2.generate_sample() : 0;
    double wave_sample = wave.is_playing() ? wave.generate_sample() : 0;
    double noise_sample = noise.is_playing() ? noise.generate_sample() : 0;

    //mix samples
    double left_sample = 0;
    double right_sample = 0;
    if (stereo_left[0]) left_sample += pulse_1_sample;
    if (stereo_right[0]) right_sample += pulse_1_sample;
    if (stereo_left[1]) left_sample += pulse_2_sample;
    if (stereo_right[1]) right_sample += pulse_2_sample;
    if (stereo_left[2]) left_sample += wave_sample;
    if (stereo_right[2]) right_sample += wave_sample;
    if (stereo_left[3]) left_sample += noise_sample;
    if (stereo_right[3]) right_sample += noise_sample;
    left_sample = dc_block(left_sample, left_cap) * 0.25 * (left_volume / 8.0);
    right_sample = dc_block(right_sample, right_cap) * 0.25 * (right_volume / 8.0);
    ui.push_audio_sample(left_sample, right_sample);
}
void Apu::tick(){
    if (enabled){
        pulse_1.tick();
        pulse_2.tick();
        wave.tick();
        noise.tick();
        if (((mem.readX(DIV_ADDR) & (1 << div_bit_to_check)) >> div_bit_to_check) != last_div_bit){
            last_div_bit ^= 1;
            if (last_div_bit == 0){
                pulse_1.frame_seq_tick(frame_step);
                pulse_2.frame_seq_tick(frame_step);
                wave.frame_seq_tick(frame_step);
                noise.frame_seq_tick(frame_step);

                frame_step = (frame_step + 1) % 8;
            }
        }
    }
    sample_timer += 1.0;
    if(sample_timer >= 87.38){
        sample_timer -= 87.38;
        this->generate_sample();
    }
}
void Apu::prepare_speed_change(){
    div_bit_to_check = (cpu.get_speed_mode() == Speed_mode::NORMAL) ? 4 : 5;
}


Apu_ss Apu::save_state() const {
    Apu_ss ss;
    ss.enabled      = enabled;
    ss.last_div_bit = last_div_bit;
    ss.sample_timer = sample_timer;
    ss.frame_step   = frame_step;
    ss.left_volume  = left_volume;
    ss.right_volume = right_volume;
    ss.left_cap     = left_cap;
    ss.right_cap    = right_cap;
    for (int i = 0; i < 4; i++){
        ss.stereo_left[i]  = stereo_left[i];
        ss.stereo_right[i] = stereo_right[i];
    }
    ss.pulse_1 = pulse_1.save_state();
    ss.pulse_2 = pulse_2.save_state();
    ss.wave    = wave.save_state();
    ss.noise   = noise.save_state();
    return ss;
}
void Apu::load_state(const Apu_ss& state){
    enabled      = state.enabled;
    last_div_bit = state.last_div_bit;
    sample_timer = state.sample_timer;
    frame_step   = state.frame_step;
    left_volume  = state.left_volume;
    right_volume = state.right_volume;
    left_cap     = state.left_cap;
    right_cap    = state.right_cap;
    for (int i = 0; i < 4; i++){
        stereo_left[i]  = state.stereo_left[i];
        stereo_right[i] = state.stereo_right[i];
    }
    pulse_1.load_state(state.pulse_1);
    pulse_2.load_state(state.pulse_2);
    wave.load_state(state.wave);
    noise.load_state(state.noise);
}