#pragma once
#include "utils.h"
class Memory;
struct Channel_regs { //Regs include triggerable values
    u8 volume;
};
// --- Save state PODs (mirrors the *_ss pattern used elsewhere) ---
struct Channel_ss {
    u16 length_limit;
    u16 length_timer;
    u8 volume_timer;
    u16 frequency;
    u32 frequency_timer;
    bool playing;
    bool dac_enabled;
    bool length_enabled;
    u8 ext_volume;
    u8 int_volume;
};
struct Pulse_channel_ss {
    Channel_ss base;
    u8 duty_idx;
    u8 ext_duty_cycle, int_duty_cycle;
    u8 ext_envelope_dir, int_envelope_dir;
    u8 ext_envelope_pace, int_envelope_pace;
};
struct Pulse_1_channel_ss {
    Pulse_channel_ss base;
    u8 ext_sweep_pace, int_sweep_pace;
    u8 sweep_direction, sweep_step, sweep_iter_counter;
    u16 shadow_frequency;
    bool negative_sweep_occurred;
};
struct Wave_channel_ss {
    Channel_ss base;
    u8 wave_pattern[16];
    u8 wave_idx;
};
enum class Noise_width;
struct Noise_channel_ss {
    Channel_ss base;
    u8 ext_envelope_dir, int_envelope_dir;
    u8 ext_envelope_pace, int_envelope_pace;
    u16 lfsr;
    u8 clock_divider, clock_shift;
    Noise_width width_mode;
};
class Channel{
    protected:
        Memory& mem;
        Channel_regs& external_regs; //reference to the storage owned by the leaf class
        Channel_regs& internal_regs;
        u16 length_limit;
        u16 length_timer;
        u8 volume_timer;

        u16 frequency;
        u32 frequency_timer;
        bool playing = false;
        bool dac_enabled = false;
        bool length_enabled = false;
        Channel(Memory& mem, Channel_regs& ext, Channel_regs& intl) : mem(mem), external_regs(ext), internal_regs(intl) {}
        void save_base(Channel_ss& ss) const;
        void load_base(const Channel_ss& ss);
    public:
        void internal_reset();
        void tick();
        void frame_seq_tick(u8 step);
        void trigger();
        double generate_sample();
        #pragma region ChannelGettersSetters
        u16 get_length_limit() const { return length_limit; }
        void set_length_limit(u16 value) { length_timer = 0; length_limit = value;}
        u8 get_volume(bool internal = true) const { return internal ? internal_regs.volume : external_regs.volume; }
        void set_volume(u8 value, bool internal = false) { if (internal) internal_regs.volume = value; external_regs.volume = value; }
        u16 get_frequency() const { return frequency; }
        void set_frequency(u16 value) { frequency = value; }
        bool is_playing() const { return playing; }
        void set_playing(bool value) { playing = value; }
        bool is_dac_enabled() const { return dac_enabled; }
        void set_dac_enabled(bool value) { dac_enabled = value; if(!value) playing = false; }
        bool is_length_enabled() const { return length_enabled; }
        void set_length_enabled(bool value) { length_enabled = value; }
        #pragma endregion
};
struct Pulse_regs: public Channel_regs{
    u8 duty_cycle;
    u8 envelope_dir;
    u8 envelope_pace;
};
class Pulse_channel : public Channel{
    private:
        u8 duty_table[4][8] = {
            {0,0,0,0,0,0,0,1}, //12.5%
            {1,0,0,0,0,0,0,1}, //25%
            {1,0,0,0,0,1,1,1}, //50%
            {0,1,1,1,1,1,1,0}  //75%
        };
    protected:
        Pulse_regs& external_regs; //widened view of the same storage Channel references
        Pulse_regs& internal_regs;
        u8 duty_idx;
        Pulse_channel(Memory& mem, Pulse_regs& ext, Pulse_regs& intl) : Channel(mem, ext, intl), external_regs(ext), internal_regs(intl) {}
    public:
        void internal_reset();
        void tick();
        void frame_seq_tick(u8 step);
        void trigger();
        double generate_sample();
        Pulse_channel_ss save_state() const;
        void load_state(const Pulse_channel_ss& ss);
        #pragma region PulseGettersSetters
        u8 get_duty_cycle(bool internal = false) const { return internal ? internal_regs.duty_cycle : external_regs.duty_cycle; }
        void set_duty_cycle(u8 value, bool internal = false) { if (internal) internal_regs.duty_cycle = value; else external_regs.duty_cycle = value; }
        u8 get_envelope_dir(bool internal = false) const { return internal ? internal_regs.envelope_dir : external_regs.envelope_dir; }
        void set_envelope_dir(u8 value, bool internal = false) { if (internal) internal_regs.envelope_dir = value; else external_regs.envelope_dir = value; }
        u8 get_envelope_pace(bool internal = false) const { return internal ? internal_regs.envelope_pace : external_regs.envelope_pace; }
        void set_envelope_pace(u8 value, bool internal = false) { if (internal) internal_regs.envelope_pace = value; else external_regs.envelope_pace = value; }

        #pragma endregion
};
struct Pulse_1_regs: public Pulse_regs{
    u8 sweep_pace;
};
class Pulse_1_channel : public Pulse_channel{
    private:
        Pulse_1_regs ext_storage; //leaf class owns the actual registers
        Pulse_1_regs int_storage;
        Pulse_1_regs& external_regs; //widened view for sweep access
        Pulse_1_regs& internal_regs;
        u8 sweep_direction;
        u8 sweep_step;
        u8 sweep_iter_counter;
        u16 shadow_frequency;
        bool negative_sweep_occurred = false;
    public:
        Pulse_1_channel(Memory& mem) : Pulse_channel(mem, ext_storage, int_storage), external_regs(ext_storage), internal_regs(int_storage) {}
        void internal_reset();
        void tick();
        void frame_seq_tick(u8 step);
        void trigger();
        Pulse_1_channel_ss save_state() const;
        void load_state(const Pulse_1_channel_ss& ss);
        #pragma region Pulse1GettersSetters
        u8 get_sweep_pace(bool internal = false) const { return internal ? internal_regs.sweep_pace : external_regs.sweep_pace; }
        void set_sweep_pace(u8 value, bool internal = false);
        u8 get_sweep_direction() const { return sweep_direction; }
        void set_sweep_direction(u8 value) { if(negative_sweep_occurred && sweep_direction && !value){set_playing(false);} sweep_direction = value;}
        u8 get_sweep_step() const { return sweep_step; }
        void set_sweep_step(u8 value) { sweep_step = value; }
        #pragma endregion
};

class Pulse_2_channel : public Pulse_channel{
    private:
        Pulse_regs ext_storage; //leaf class owns the actual registers
        Pulse_regs int_storage;
    public:
        Pulse_2_channel(Memory& mem) : Pulse_channel(mem, ext_storage, int_storage) {}
};

struct Wave_regs: public Channel_regs{

};
class Wave_channel : public Channel{
    private:
        Wave_regs ext_storage; //leaf class owns the actual registers
        Wave_regs int_storage;
        Wave_regs& external_regs;
        Wave_regs& internal_regs;
        u8 wave_pattern[16];
        u8 wave_idx;
    public:
        Wave_channel(Memory& mem) : Channel(mem, ext_storage, int_storage), external_regs(ext_storage), internal_regs(int_storage) {}
        void internal_reset();
        void tick();
        void frame_seq_tick(u8 step);
        void trigger();
        double generate_sample();
        Wave_channel_ss save_state() const;
        void load_state(const Wave_channel_ss& ss);
        #pragma region WaveGettersSetters
        u8 get_volume() const { return Channel::get_volume(true); }
        void set_volume(u8 value) { Channel::set_volume(value, true); }
        u8 get_wave_pattern(u8 idx) const { return is_playing()? wave_pattern[idx] : 0xFF; }
        void set_wave_pattern(u8 idx, u8 value) { if (!is_playing()) wave_pattern[idx] = value; }
        u8 get_wave_idx() const { return wave_idx; }
        void set_wave_idx(u8 value) { wave_idx = value; }
        #pragma endregion
};

enum class Noise_width{
    M7,
    M15
};
struct Noise_regs: public Channel_regs{
    u8 envelope_dir;
    u8 envelope_pace;
};
class Noise_channel : public Channel{
    private:
        Noise_regs ext_storage; //leaf class owns the actual registers
        Noise_regs int_storage;
        Noise_regs& external_regs;
        Noise_regs& internal_regs;
        u16 lfsr = 0;
        u8 clock_divider = 0;
        u8 clock_shift = 0;
        Noise_width width_mode = Noise_width::M15;
    public:
        Noise_channel(Memory& mem) : Channel(mem, ext_storage, int_storage), external_regs(ext_storage), internal_regs(int_storage) {}
        void internal_reset();
        void tick();
        void frame_seq_tick(u8 step);
        void trigger();
        double generate_sample();
        Noise_channel_ss save_state() const;
        void load_state(const Noise_channel_ss& ss);
        #pragma region NoiseGettersSetters
        u8 get_envelope_dir(bool internal = false) const { return internal ? internal_regs.envelope_dir : external_regs.envelope_dir; }
        void set_envelope_dir(u8 value, bool internal = false) { if (internal) internal_regs.envelope_dir = value; else external_regs.envelope_dir = value; }
        u8 get_envelope_pace(bool internal = false) const { return internal ? internal_regs.envelope_pace : external_regs.envelope_pace; }
        void set_envelope_pace(u8 value, bool internal = false) { if (internal) internal_regs.envelope_pace = value; else external_regs.envelope_pace = value; }
        u8 get_clock_divider() const { return clock_divider; }
        void set_clock_divider(u8 value) { clock_divider = value; }
        u8 get_clock_shift() const { return clock_shift; }
        void set_clock_shift(u8 value) { clock_shift = value; }
        Noise_width get_width_mode() const { return width_mode; }
        void set_width_mode(Noise_width value) { width_mode = value; }
        #pragma endregion
};
