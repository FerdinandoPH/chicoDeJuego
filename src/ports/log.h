#pragma once

// Where the emulator's messages end up. A terminal is not a given: on a console
// there is no stdout to write to, so the core never touches it directly and just
// hands the finished message over to the backend, which decides whether it goes
// to a terminal, to a file or nowhere at all.
enum class Log_level { INFO, WARN, ERROR };

struct ILog {
    virtual ~ILog() = default;

    // msg is already formatted and already carries whatever newlines the core
    // wanted: some messages are deliberately left unterminated so the next one
    // continues the same line ("Saving state... " / "Done.\n"). The backend must
    // not add any of its own.
    virtual void write(Log_level level, const char* msg) = 0;
};
