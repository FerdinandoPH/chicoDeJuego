#pragma once
#include "utils.h"
#include "input.h"
#include "video.h"

// Event sink. This is the ONLY port that goes platform -> core: the other three
// are called by the core. The backend translates its native events and reports
// them here, without needing to know about Controller or Debugger.
struct IEvent_sink {
    virtual ~IEvent_sink() = default;

    virtual void on_key(Host_key k, Controller_event_type t) = 0;
    virtual void on_quit() = 0;
    virtual void on_break() = 0;                     // ESC -> debugger menu
    virtual void on_aux_closed(DebugWindowType w) = 0;
};

// Odds and ends provided by the host: time, events and opening files.
struct ISystem {
    virtual ~ISystem() = default;

    virtual void delay_ns(u64 ns) = 0;
    virtual void pump_events(IEvent_sink& sink) = 0;
    virtual bool open_with_default_app(const char* path) = 0;
};
