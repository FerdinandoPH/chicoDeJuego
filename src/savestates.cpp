#include "savestates.h"

SaveStateManager::SaveStateManager(Cpu* cpu, Timer* timer, Ppu* ppu, Memory* mem, Dma* dma, Ui* ui, int& ticks, int& ticks_since_last_sync)
    : cpu(cpu), timer(timer), ppu(ppu), mem(mem), dma(dma), ui(ui), ticks(ticks), ticks_since_last_sync(ticks_since_last_sync){}

// --- Binary I/O helpers ---

template<typename T>
static void write_pod(FILE* f, const T& val) {
    fwrite(&val, sizeof(T), 1, f);
}

template<typename T>
static void read_pod(FILE* f, T& val) {
    fread(&val, sizeof(T), 1, f);
}

static void write_string(FILE* f, const std::string& s) {
    u32 size = static_cast<u32>(s.size());
    fwrite(&size, sizeof(size), 1, f);
    fwrite(s.data(), 1, size, f);
}

static void read_string(FILE* f, std::string& s) {
    u32 size;
    fread(&size, sizeof(size), 1, f);
    s.resize(size);
    fread(s.data(), 1, size, f);
}

template<typename T>
static void write_vector(FILE* f, const std::vector<T>& v) {
    u32 size = static_cast<u32>(v.size());
    fwrite(&size, sizeof(size), 1, f);
    if (size > 0) fwrite(v.data(), sizeof(T), size, f);
}

template<typename T>
static void read_vector(FILE* f, std::vector<T>& v) {
    u32 size;
    fread(&size, sizeof(size), 1, f);
    v.resize(size);
    if (size > 0) fread(v.data(), sizeof(T), size, f);
}

template<typename T>
static void write_deque(FILE* f, const std::deque<T>& d) {
    u32 size = static_cast<u32>(d.size());
    fwrite(&size, sizeof(size), 1, f);
    for (const auto& item : d) fwrite(&item, sizeof(T), 1, f);
}

template<typename T>
static void read_deque(FILE* f, std::deque<T>& d) {
    u32 size;
    fread(&size, sizeof(size), 1, f);
    d.clear();
    for (u32 i = 0; i < size; i++) {
        T item;
        fread(&item, sizeof(T), 1, f);
        d.push_back(item);
    }
}

// --- Per-type serializers for non-trivial structs ---

static void write_memory_ss(FILE* f, const Memory_ss& ss) {
    write_pod(f, ss.mbc_type);
    write_pod(f, ss.mbc_data);
    write_pod(f, ss.current_rom0_bank);
    write_pod(f, ss.current_rom1_bank);
    write_pod(f, ss.current_ram_bank);
    write_pod(f, ss.modifiable_mem);
    write_vector(f, ss.ram);
}

static void read_memory_ss(FILE* f, Memory_ss& ss) {
    read_pod(f, ss.mbc_type);
    read_pod(f, ss.mbc_data);
    read_pod(f, ss.current_rom0_bank);
    read_pod(f, ss.current_rom1_bank);
    read_pod(f, ss.current_ram_bank);
    read_pod(f, ss.modifiable_mem);
    read_vector(f, ss.ram);
}

static void write_fifo_ss(FILE* f, const Pixel_FIFO_ss& ss) {
    write_pod(f, ss.lx);
    write_pod(f, ss.ly);
    write_pod(f, ss.triggered_wx);
    write_pod(f, ss.win_ly);
    write_pod(f, ss.pixels_to_discard);
    write_pod(f, ss.increase_win_ly);
    write_pod(f, ss.waiting_for_sprite);
    write_pod(f, ss.wx_cond);
    write_pod(f, ss.wy_cond);
    write_pod(f, ss.window_active);
    write_pod(f, ss.sprites_in_pixel_done);
    write_deque(f, ss.pixels);
    write_deque(f, ss.obj_pixels);
    write_deque(f, ss.sprites_in_pixel);
}

static void read_fifo_ss(FILE* f, Pixel_FIFO_ss& ss) {
    read_pod(f, ss.lx);
    read_pod(f, ss.ly);
    read_pod(f, ss.triggered_wx);
    read_pod(f, ss.win_ly);
    read_pod(f, ss.pixels_to_discard);
    read_pod(f, ss.increase_win_ly);
    read_pod(f, ss.waiting_for_sprite);
    read_pod(f, ss.wx_cond);
    read_pod(f, ss.wy_cond);
    read_pod(f, ss.window_active);
    read_pod(f, ss.sprites_in_pixel_done);
    read_deque(f, ss.pixels);
    read_deque(f, ss.obj_pixels);
    read_deque(f, ss.sprites_in_pixel);
}

static void write_ppu_ss(FILE* f, const Ppu_ss& ss) {
    write_pod(f, ss.fetcher);
    write_fifo_ss(f, ss.fifo);
    write_pod(f, ss.oam);
    write_pod(f, ss.line_oam);
    write_pod(f, ss.sprites_in_line);
    write_pod(f, ss.is_on);
    write_pod(f, ss.startup);
    write_pod(f, ss.ppu_mode);
    write_pod(f, ss.line_ticks);
}

static void read_ppu_ss(FILE* f, Ppu_ss& ss) {
    read_pod(f, ss.fetcher);
    read_fifo_ss(f, ss.fifo);
    read_pod(f, ss.oam);
    read_pod(f, ss.line_oam);
    read_pod(f, ss.sprites_in_line);
    read_pod(f, ss.is_on);
    read_pod(f, ss.startup);
    read_pod(f, ss.ppu_mode);
    read_pod(f, ss.line_ticks);
}

// --- SaveStateManager ---

void SaveStateManager::save_state(){
    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        fprintf(stderr, "Failed to save state to %s\n", filename.c_str());
        return;
    }

    Save_state state;
    state.cpu_state              = cpu->save_state();
    state.timer_state            = timer->save_state();
    state.ppu_state              = ppu->save_state();
    state.mem_state              = mem->save_state();
    state.dma_state              = dma->save_state();
    state.ui_state               = ui->save_state();
    state.ticks                  = ticks;
    state.ticks_since_last_sync  = ticks_since_last_sync;
    state.sha256                 = mem->get_sha256();

    write_pod(file, state.cpu_state);
    write_pod(file, state.timer_state);
    write_ppu_ss(file, state.ppu_state);
    write_memory_ss(file, state.mem_state);
    write_pod(file, state.dma_state);
    write_pod(file, state.ui_state);
    write_pod(file, state.ticks);
    write_pod(file, state.ticks_since_last_sync);
    write_string(file, state.sha256);

    fclose(file);
}

void SaveStateManager::load_state(){
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        fprintf(stderr, "Failed to load state from %s\n", filename.c_str());
        return;
    }

    Save_state state;
    read_pod(file, state.cpu_state);
    read_pod(file, state.timer_state);
    read_ppu_ss(file, state.ppu_state);
    read_memory_ss(file, state.mem_state);
    read_pod(file, state.dma_state);
    read_pod(file, state.ui_state);
    read_pod(file, state.ticks);
    read_pod(file, state.ticks_since_last_sync);
    read_string(file, state.sha256);

    fclose(file);

    if (state.sha256 != mem->get_sha256()) {
        fprintf(stderr, "Warning: The ROM hash does not match the one in the save state. The state may not load correctly.\n");
    }

    cpu->load_state(state.cpu_state);
    timer->load_state(state.timer_state);
    ppu->load_state(state.ppu_state);
    mem->load_state(state.mem_state);
    dma->load_state(state.dma_state);
    ui->load_state(state.ui_state);
    ticks                 = state.ticks;
    ticks_since_last_sync = state.ticks_since_last_sync;
}
