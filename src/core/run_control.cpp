#include "run_control.h"

void Run_control::request_break(){
    this->break_requested.store(true, std::memory_order_relaxed);
}

bool Run_control::take_break_request(){
    // exchange and not a load+store: two threads could be asking at the same
    // time (ESC and Ctrl-C), and the break must happen exactly once.
    return this->break_requested.exchange(false, std::memory_order_relaxed);
}

bool Run_control::is_paused(){
    return this->paused.load(std::memory_order_acquire);
}

void Run_control::wait_until_paused(){
    std::unique_lock<std::mutex> lock(this->paused_mutex);
    this->paused_cv.wait(lock, [this]{ return this->paused.load(std::memory_order_acquire); });
}

void Run_control::enter_paused(){
    {
        std::scoped_lock<std::mutex> lock(this->paused_mutex);
        this->paused.store(true, std::memory_order_release);
    }
    this->paused_cv.notify_all();
}

void Run_control::leave_paused(){
    {
        std::scoped_lock<std::mutex> lock(this->paused_mutex);
        this->paused.store(false, std::memory_order_release);
    }
    this->paused_cv.notify_all();
}
