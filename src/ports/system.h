#pragma once
#include "utils.h"
#include "input.h"
#include "video.h"
#include <string>

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

    // Asks the user to pick a file, blocking until they answer. filter_name and
    // filter_pattern describe what to offer ("Game Boy ROMs", "gb;gbc").
    // Returns false if the user cancelled, and also on a backend that has no way
    // to ask (a console, libretro): the core then just reports that it has no ROM.
    //
    // It is called only after IVideo::init, and that is deliberate. The backend is
    // expected to hand the dialog its own window as the parent: on Windows an
    // ownerless dialog leaves the process with no window to give the foreground
    // back to when it closes, and every window opened afterwards comes up behind
    // the other applications.
    virtual bool pick_file(const char* title, const char* filter_name,
                           const char* filter_pattern, std::string& out) = 0;
};
