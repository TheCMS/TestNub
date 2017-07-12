#ifndef AUTOTESTLOGGING_H
#define AUTOTESTLOGGING_H

enum eAutoLogLevels
{
    ALL_SPECIAL=-1,
    ALL_FATAL=0,
    ALL_ERROR,
    ALL_WARNING,
    ALL_INFO,
    ALL_DEBUG,
    ALL_VERBOSE,
};

typedef void (*autoTestLoggingCb)(eAutoLogLevels level, const char* ctx, const char* fmt, ...);


static inline const char* LogLevelToStr(eAutoLogLevels level)
{
    switch(level)
    {
    case ALL_FATAL: return "FATAL";
    case ALL_ERROR: return "ERROR";
    case ALL_WARNING: return "WARNN";
    case ALL_INFO: return "INFFO";
    case ALL_DEBUG: return "DEBUG";
    case ALL_VERBOSE: return "VRBOS";
    case ALL_SPECIAL: return "SPECL";
    default: return "UNKNW";
    }
}


#endif // AUTOTESTLOGGING_H
