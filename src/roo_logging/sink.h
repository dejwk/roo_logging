#pragma once

#include "roo_logging.h"

namespace roo_logging {

// Used to send logs to some other kind of destination
// Users should subclass LogSink and override send to do whatever they want.
class LogSink {
 public:
  virtual ~LogSink() = default;

  // Sink's logging logic (message_len is such as to exclude '\n' at the end).
  // This method can't use LOG() or CHECK() as logging.
  virtual void send(LogSeverity severity, const char* full_filename,
                    const char* base_filename, int line,
                    roo_time::Uptime uptime, roo_time::WallTime walltime,
                    const char* message, size_t message_len) = 0;

  // Redefine this to implement waiting for
  // the sink's logging logic to complete.
  // It will be called after each send() returns,
  // but before that LogMessage exits or crashes.
  // By default this function does nothing.
  virtual void WaitTillSent() {}
};

extern LogSink* sink_;

void SetSink(LogSink* sink);

void MaybeLogToSink(LogSeverity severity, const char* full_filename,
                    const char* base_filename, int line,
                    roo_time::Uptime uptime, roo_time::WallTime walltime,
                    const char* message, size_t message_len);

}  // namespace roo_logging