#include "roo_logging/config.h"

ROO_FLAG(roo_logging::WallTimeClockPtr, roo_logging_wall_time_clock, nullptr);
ROO_FLAG(roo_time::TimeZone, roo_logging_timezone, roo_time::timezone::UTC);
ROO_FLAG(bool, roo_logging_prefix, true);
ROO_FLAG(bool, roo_logging_colorlogtostderr, true);
ROO_FLAG(uint8_t, roo_logging_minloglevel, 0);
