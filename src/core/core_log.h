#pragma once
#include "log.h"

// printf-style front end to the ILog port. This is what the core calls instead
// of printf/std::cout: the message is formatted here (so every backend gets
// plain text and nothing else) and handed over to whichever ILog the backend
// provided. The header is named core_log.h and not log.h because ports/log.h
// already takes that name and both directories are on the include path.
//
// As with the printf calls these replaced, the newline is part of the message.
void log_info(const char* fmt, ...);
void log_warn(const char* fmt, ...);
void log_error(const char* fmt, ...);
