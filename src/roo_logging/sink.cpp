#include "roo_logging/sink.h"

namespace roo_logging {

LogSink* sink_ = nullptr;

void SetSink(LogSink* sink) { sink_ = sink; }

void MaybeLogToSink(LogSeverity severity, const char* full_filename,
                    const char* base_filename, int line,
                    roo_time::Uptime uptime, roo_time::WallTime walltime,
                    const char* message, size_t message_len) {
  if (sink_ != nullptr) {
    sink_->send(severity, full_filename, base_filename, line, uptime, walltime,
                message, message_len);
  }
}

};  // namespace roo_logging
