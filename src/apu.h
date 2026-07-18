#pragma once
#include "utils.h"
#include "apu_channels.h"
#include "hw_reg_def.h"
class Memory;
class Ui;
enum class Audio_reg_write_origin{CPU, APU};
struct Apu_ss {
    bool enabled;
    u8 last_div_bit;
    double sample_timer;
    u8 frame_step;
    u8 left_volume, right_volume;
    double left_cap, right_cap;
    bool stereo_left[4];
    bool stereo_right[4];
    Pulse_1_channel_ss pulse_1;
    Pulse_channel_ss pulse_2;
    Wave_channel_ss wave;
    Noise_channel_ss noise;
};
class Apu{
    private:
        Memory& mem;
        Ui& ui;
        bool enabled = true;
        u8 last_div_bit = 0;
        u8 div_bit_to_check = 4;
        double sample_timer = 0;
        u8 frame_step = 0;
        u8 left_volume = 8;
        u8 right_volume = 8;
        double left_cap = 0;
        double right_cap = 0;
        Pulse_1_channel pulse_1;
        Pulse_2_channel pulse_2;
        Wave_channel wave;
        Noise_channel noise;
        bool stereo_left[4] = {false, false, false, false};
        bool stereo_right[4] = {false, false, false, false};
        void generate_sample();
        double dc_block(double in, double& cap);
    public:
        Apu(Memory& mem, Ui& ui);
        void internal_reset();
        u8 write(u16 addr, u8 data);
        void tick();
        Apu_ss save_state() const;
        void load_state(const Apu_ss& state);
};