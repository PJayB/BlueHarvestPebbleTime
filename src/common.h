#pragma once

#ifndef WIN32
#   include <pebble.h>
#   define ASSERT(x) if (x) {} else { APP_LOG(APP_LOG_LEVEL_ERROR, "Assert! " #x); }
#else
#   include <stdint.h>
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   define ASSERT(x) if (x) {} else { DebugBreak(); }
#endif
