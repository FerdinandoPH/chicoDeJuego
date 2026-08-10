#include "run_control.h"

void Run_control::request_break(){
    this->break_requested.store(true, std::memory_order_relaxed);
}

bool Run_control::take_break_request(){
    // This runs once per emulated instruction, and almost every time the answer
    // is no: the plain load is the fast path, and it compiles to an ordinary
    // read. Only when there really is a request do we pay for the atomic
    // read-modify-write, which is what guarantees the break happens exactly once
    // even if ESC and Ctrl-C arrive together.
    if (!this->break_requested.load(std::memory_order_relaxed))
        return false;
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
