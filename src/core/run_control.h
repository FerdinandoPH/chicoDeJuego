#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>

// Handshake between the emulation thread and whoever wants it to hold still.
//
// The emulation thread is the only one that ever stops itself, and it only does
// so at an instruction boundary: that is the one point where the machine is
// consistent enough to be looked at. Everyone else (the ESC key, Ctrl-C) can
// only *ask*, with request_break, and the request is picked up on the next
// boundary. Before this existed the debug level was written straight from the
// UI thread while the emulation thread was reading it, which is a data race.
//
// While the menu runs, the emulation thread sits inside enter_paused/leave_paused,
// so a backend whose menu lives on another thread (an on-screen one, drawn by
// the render thread) can use is_paused/wait_until_paused to know when it is safe
// to take over.
class Run_control{
    private:
        std::atomic<bool> break_requested{false};
        std::atomic<bool> paused{false};
        std::mutex paused_mutex;
        std::condition_variable paused_cv;
    public:
        // --- Any thread ---
        void request_break();
        bool is_paused();
        void wait_until_paused();

        // --- Emulation thread only ---
        bool take_break_request();  // true once per request, and clears it
        void enter_paused();
        void leave_paused();
};
