// Based on Google logging library, subject to the copyright below.
//
// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "roo_logging/log_message.h"

#if (defined __FREERTOS)
#include "freertos/task.h"
#elif (defined __linux__)
#include <pthread.h>
#endif

#include "roo_logging/exit.h"
#include "roo_logging/sink.h"
#include "roo_logging/stderr.h"
#include "roo_threads.h"
#include "roo_threads/mutex.h"

namespace roo_logging {

void WaitForSinks(LogMessage::LogMessageData* data) {}

// For now we're going to assume that logging will only be called from the
// user thread.
// char log_buffer[kMaxLogMessageLen];

// Has the user called SetExitOnDFatal(true)?
static bool exit_on_dfatal = true;

// A mutex that allows only one thread to log at a time, to keep things from
// getting jumbled.  Some other very uncommon logging operations (like
// changing the destination file for log messages of a given severity) also
// lock this mutex.  Please be sure that anybody who might possibly need to
// lock it does so.
//
// Using a function, to make sure that the mutex is initialized even if
// LOG(INFO) gets called from a static initializer itself. (Otherwise, there's
// undefined initialization order).
static roo::mutex& log_mutex() {
  static roo::mutex m;
  return m;
};

// Number of messages sent at each severity.  Under log_mutex.
int64_t num_messages_[NUM_SEVERITIES] = {0, 0, 0, 0};

// // Globally disable log writing (if disk is full)
// static bool stop_writing = false;

const char* const LogSeverityNames[NUM_SEVERITIES] = {"INFO", "WARNING",
                                                      "ERROR", "FATAL"};

namespace {

const char* const_basename(const char* filepath) {
  const char* base = strrchr(filepath, '/');
  return base ? (base + 1) : filepath;
}

}  // namespace

struct LogMessage::LogMessageData {
  LogMessageData() : stream_(message_text_, kMaxLogMessageLen) {}

  //   int preserved_errno_;      // preserved errno
  // Buffer space; contains complete message text.
  char message_text_[kMaxLogMessageLen + 1];
  Stream stream_;
  char severity_;  // What level is this LogMessage logged at?
  int line_;       // line number where logging call is.
  void (LogMessage::*send_method_)();  // Call this in destructor to send
  //   union {  // At most one of these is used: union to keep the size low.
  //     LogSink* sink_;             // NULL or sink to send message to
  //     std::vector<std::string>* outvec_; // NULL or vector to push message
  //     onto std::string* message_;             // NULL or string to write
  //     message into
  //   };
  roo_time::Uptime uptime_;      // Time of creation of LogMessage
  roo_time::WallTime walltime_;  // Time of creation of LogMessage
  size_t num_prefix_chars_;      // # of chars of prefix in this message
  size_t num_chars_to_log_;      // # of chars of msg to send to log
  //   size_t num_chars_to_syslog_;  // # of chars of msg to send to syslog
  const char* basename_;          // basename of file that called LOG
  const char* fullname_;          // fullname of file that called LOG
  bool has_been_flushed_;         // false => data has not been flushed
  bool first_fatal_;              // true => this was first fatal msg
  bool from_static_initializer_;  // true => logging before main()

 private:
  LogMessageData(const LogMessageData&);
  void operator=(const LogMessageData&);
};

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       int ctr, void (LogMessage::*send_method)())
    : allocated_(NULL) {
  Init(file, line, severity, send_method);
  data_->stream_.set_ctr(ctr);
}

LogMessage::LogMessage(const char* file, int line, const CheckOpString& result)
    : allocated_(NULL) {
  Init(file, line, ROO_LOGGING_FATAL, &LogMessage::SendToLog);
  stream() << "Check failed: " << (*result.str_) << " ";
}

LogMessage::LogMessage(const char* file, int line) : allocated_(NULL) {
  Init(file, line, ROO_LOGGING_INFO, &LogMessage::SendToLog);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : allocated_(NULL) {
  Init(file, line, severity, &LogMessage::SendToLog);
}

// LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
//                        LogSink* sink, bool also_send_to_log)
//     : allocated_(NULL) {
//   Init(file, line, severity,
//        also_send_to_log ? &LogMessage::SendToSinkAndLog
//                         : &LogMessage::SendToSink);
//   data_->sink_ = sink;  // override Init()'s setting to NULL
// }

// LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
//                        vector<string>* outvec)
//     : allocated_(NULL) {
//   Init(file, line, severity, &LogMessage::SaveOrSendToLog);
//   data_->outvec_ = outvec;  // override Init()'s setting to NULL
// }

// LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
//                        string* message)
//     : allocated_(NULL) {
//   Init(file, line, severity, &LogMessage::WriteToStringAndLog);
//   data_->message_ = message;  // override Init()'s setting to NULL
// }

void LogMessage::Init(const char* file, int line, LogSeverity severity,
                      void (LogMessage::*send_method)()) {
  allocated_ = NULL;
  if (severity != ROO_LOGGING_FATAL || !exit_on_dfatal) {
    // #ifdef GLOG_THREAD_LOCAL_STORAGE
    //     // No need for locking, because this is thread local.
    //     if (thread_data_available) {
    //       thread_data_available = false;
    //       data_ = new (&thread_msg_data) LogMessageData;
    //     } else {
    //       allocated_ = new LogMessageData();
    //       data_ = allocated_;
    //     }
    // #else   // !defined(GLOG_THREAD_LOCAL_STORAGE)
    allocated_ = new LogMessageData();
    data_ = allocated_;
    // #endif  // defined(GLOG_THREAD_LOCAL_STORAGE)
    data_->first_fatal_ = false;
  } else {
    allocated_ = new LogMessageData();
    data_ = allocated_;
    // #endif  // defined(GLOG_THREAD_LOCAL_STORAGE)
    data_->first_fatal_ = false;
    // MutexLock l(&fatal_msg_lock);
    // if (fatal_msg_exclusive) {
    //   fatal_msg_exclusive = false;
    //   data_ = &fatal_msg_data_exclusive();
    //   data_->first_fatal_ = true;
    // } else {
    //   data_ = &fatal_msg_data_shared();
    //   data_->first_fatal_ = false;
    // }
  }

  //   stream().fill('0');
  //   data_->preserved_errno_ = errno;
  data_->severity_ = severity;
  data_->line_ = line;
  data_->send_method_ = send_method;
  //   data_->sink_ = NULL;
  //   data_->outvec_ = NULL;

  data_->uptime_ = roo_time::Uptime::Now();

  data_->num_chars_to_log_ = 0;
  data_->basename_ = const_basename(file);
  data_->fullname_ = file;
  data_->has_been_flushed_ = false;
  data_->from_static_initializer_ = false;

  if (GET_ROO_FLAG(roo_logging_prefix)) {
    stream() << LogSeverityNames[severity][0];
    roo_time::WallTimeClock* clock = GET_ROO_FLAG(roo_logging_wall_time_clock);
    if (clock == nullptr) {
      stream() << data_->uptime_ << " ";
    } else {
      data_->walltime_ = clock->now();
      roo_time::TimeZone tz = GET_ROO_FLAG(roo_logging_timezone);
      roo_time::DateTime dt(data_->walltime_, tz);
      stream() << dt;
      stream().write(' ');
    }
#if (defined __FREERTOS)
    TaskHandle_t tHandle = xTaskGetCurrentTaskHandle();
    // Can be null if called from a static initializer.
    if (tHandle != nullptr) {
      char* tName = pcTaskGetName(tHandle);
      stream() << tName << '(' << tHandle << ") ";
    } else {
      data_->from_static_initializer_ = true;
    }
#elif (defined __linux__)
    {
      char buf[64];
      pthread_getname_np(pthread_self(), buf, 64);
      if (buf[0] != 0) {
        stream() << buf << " ";
      } else {
        data_->static_initializer_ = true;
      }
    }
#endif
    stream() << data_->basename_ << ":" << data_->line_ << "] ";
  }
  data_->num_prefix_chars_ = data_->stream_.pcount();

  //   if (!GET_ROO_GLOG_FLAG(log_backtrace_at).empty()) {
  //     char fileline[128];
  //     snprintf(fileline, sizeof(fileline), "%s:%d", data_->basename_, line);
  // #ifdef HAVE_STACKTRACE
  //     if (!strcmp(GET_ROO_GLOG_FLAG(log_backtrace_at).c_str(), fileline)) {
  //       string stacktrace;
  //       DumpStackTraceToString(&stacktrace);
  //       stream() << " (stacktrace:\n" << stacktrace << ") ";
  //     }
  // #endif
  //   }
}

Stream& LogMessage::stream() { return data_->stream_; }

LogMessage::~LogMessage() {
  Flush();
  // #ifdef GLOG_THREAD_LOCAL_STORAGE
  //   if (data_ == static_cast<void*>(thread_msg_data)) {
  //     data_->~LogMessageData();
  //     thread_data_available = true;
  //   } else {
  //     delete allocated_;
  //   }
  // #else   // !defined(GLOG_THREAD_LOCAL_STORAGE)
  delete allocated_;
  allocated_ = nullptr;
  // #endif  // defined(GLOG_THREAD_LOCAL_STORAGE)
}

// Flush buffered message, called by the destructor, or any other function
// that needs to synchronize the log.
void LogMessage::Flush() {
  if (data_->has_been_flushed_ ||
      data_->severity_ < GET_ROO_FLAG(roo_logging_minloglevel)) {
    return;
  }

  data_->num_chars_to_log_ = data_->stream_.pos_;  // data_->stream_.pcount();

  // Do we need to add a \n to the end of this message?
  bool append_newline =
      (data_->message_text_[data_->num_chars_to_log_ - 1] != '\n');

  if (append_newline) {
    data_->message_text_[data_->num_chars_to_log_++] = '\n';
  }

  // Prevent any subtle race conditions by wrapping a mutex lock around
  // the actual logging action per se.
  {
    roo::lock_guard<roo::mutex> l{log_mutex()};
    (this->*(data_->send_method_))();
    ++num_messages_[static_cast<int>(data_->severity_)];
  }
  // LogDestination::WaitForSinks(data_);

  // Note that this message is now safely logged.  If we're asked to flush
  // again, as a result of destruction, say, we'll do nothing on future calls.
  data_->has_been_flushed_ = true;
}

void LogMessage::SendToLog() /*EXCLUSIVE_LOCKS_REQUIRED(log_mutex)*/ {
  // Messages of a given severity get logged to lower severity logs, too

  if (true) {
    // log this message to all log files of severity <= severity_
    // LogToAllLogfiles(data_->severity_, data_->timestamp_,
    // data_->message_text_,
    //                  data_->num_chars_to_log_);
    data_->message_text_[data_->num_chars_to_log_] = '\0';
    MaybeLogToStderr(data_->severity_, data_->message_text_,
                     data_->num_chars_to_log_, data_->from_static_initializer_);
    MaybeLogToSink(data_->severity_, data_->fullname_, data_->basename_,
                   data_->line_, data_->uptime_, data_->walltime_,
                   data_->message_text_ + data_->num_prefix_chars_,
                   (data_->num_chars_to_log_ - data_->num_prefix_chars_ - 1));
    // NOTE: -1 removes trailing \n
  }

  // If we log a FATAL message, flush all the log destinations, then toss
  // a signal for others to catch. We leave the logs in a state that
  // someone else can use them (as long as they flush afterwards)
  if (data_->severity_ == ROO_LOGGING_FATAL && exit_on_dfatal) {
    // if (GET_ROO_FLAG(alsologtologfiles)) {
    //   // for (int i = 0; i < NUM_SEVERITIES; ++i) {
    //   //   if (LogDestination::log_destinations_[i])
    //   //     LogDestination::log_destinations_[i]->logger_->Write(true, 0,
    //   "", 0);
    //   // }
    // }
    WaitForSinks(data_);

    const char* message = "*** Check failure stack trace: ***\n";
    if (fwrite(message, strlen(message), 1, stderr) < 0) {
      // Ignore errors.
    }
    Fail();
  }
}

LogMessageFatal::LogMessageFatal(const char* file, int line)
    : LogMessage(file, line, ROO_LOGGING_FATAL) {}

LogMessageFatal::LogMessageFatal(const char* file, int line,
                                 const CheckOpString& result)
    : LogMessage(file, line, result) {}

LogMessageFatal::~LogMessageFatal() {
  Flush();
  Fail();
}

}  // namespace roo_logging