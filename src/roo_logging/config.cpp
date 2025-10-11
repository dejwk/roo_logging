#include "roo_logging/config.h"

#ifndef ROO_LOGGING_COLORLOGTOSTDERR
#define ROO_LOGGING_COLORLOGTOSTDERR false
#endif
#ifndef ROO_LOGGING_PREFIX
#define ROO_LOGGING_PREFIX true
#endif
#ifndef ROO_LOGGING_MINLOGLEVEL
#define ROO_LOGGING_MINLOGLEVEL 0
#endif

ROO_FLAG(roo_logging::WallTimeClockPtr, roo_logging_wall_time_clock, nullptr);
ROO_FLAG(roo_time::TimeZone, roo_logging_timezone, roo_time::timezone::UTC);
ROO_FLAG(bool, roo_logging_prefix, ROO_LOGGING_PREFIX);
ROO_FLAG(bool, roo_logging_colorlogtostderr, ROO_LOGGING_COLORLOGTOSTDERR);
ROO_FLAG(uint8_t, roo_logging_minloglevel, ROO_LOGGING_MINLOGLEVEL);
