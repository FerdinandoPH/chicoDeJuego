#include "core_log.h"
#include "backend.h"
#include <cstdarg>
#include <cstdio>
#include <string>

// Resolved the first time something logs, not at static init time: the backend's
// factory may need its own libraries up before it can hand out an ILog.
static ILog* get_log(){
    static ILog* log = create_log();
    return log;
}

// vsnprintf is called twice on purpose: the first pass only asks how long the
// message is, so a long dump (a register dump, the breakpoint list) is never
// silently truncated by a fixed-size buffer.
static void log_va(Log_level level, const char* fmt, va_list args){
    va_list args_copy;
    va_copy(args_copy, args);
    int size = std::vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    if (size < 0) return;

    std::string msg(size, '\0');
    std::vsnprintf(msg.data(), size + 1, fmt, args);
    get_log()->write(level, msg.c_str());
}

void log_info(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    log_va(Log_level::INFO, fmt, args);
    va_end(args);
}
void log_warn(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    log_va(Log_level::WARN, fmt, args);
    va_end(args);
}
void log_error(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    log_va(Log_level::ERROR, fmt, args);
    va_end(args);
}
