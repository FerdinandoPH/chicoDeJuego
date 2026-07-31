#pragma once
#include <cstddef>

// Audio output. The core accumulates samples in its own buffer and hands them
// over in blocks (~90 times/s), not one at a time.
struct IAudio {
    virtual ~IAudio() = default;

    virtual bool init(int freq, int channels) = 0;
    virtual void shutdown() = 0;

    // n_floats is the total number of floats (samples * channels).
    virtual void submit(const float* interleaved, size_t n_floats) = 0;

    // Bytes not played back yet. Emu_sync uses it as its clock to keep in sync.
    virtual int  queued_bytes() = 0;
    virtual void clear() = 0;
};
