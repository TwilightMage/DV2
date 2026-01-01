#pragma once

#if 1
#include "Logging/LogMacros.h"

#define debug(...) UE_LOG(LogTemp, Display, __VA_ARGS__)
#define info(...) UE_LOG(LogTemp, Display, __VA_ARGS__)
#define warn(...) UE_LOG(LogTemp, Warning, __VA_ARGS__)
#define error(...) { UE_LOG(LogTemp, Error, __VA_ARGS__); return -1; }
#define error_v(val, ...) { UE_LOG(LogTemp, Error, __VA_ARGS__); return val; }
#define error_s(...) UE_LOG(LogTemp, Error, __VA_ARGS__);
#define fatal(...) error(__VA_ARGS__)
#define fatal_v(val, ...) error_v(val, __VA_ARGS__)
#else
//#define debug(...) mocDbg()
//#define info(...) mocDbg()
//#define warn(...) mocDbg()
//#define error(...) mocDbg()
//#define syserror(...) mocDbg()
//#define fatal(...) mocDbg()
//#define sysfatal(...) mocDbg()
//inline void mocDbg () {}
#endif
