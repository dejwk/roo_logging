// #include "roo_logging/logfile.h"

// #include "roo_flags.h"
// #include "roo_logging.h"
// #include "roo_logging/log_severity.h"
// #include "roo_time.h"

// // Buffer log messages logged at this level or lower means don't buffer; 0 means
// // buffer INFO only, etc.
// ROO_FLAG(int8_t, roo_logging_logbuflevel, 0);

// // Log messages go to logfiles (in addition to possibly stderr).
// ROO_FLAG(bool, roo_logging_alsologtologfiles, false);

// // Approx. maximum log file size (in MB).
// ROO_FLAG(int32_t, roo_logging_max_log_size, 1);

// namespace roo_logging {

// namespace {

// static constexpr uint32_t kRolloverAttemptFrequency = 32;

// int32_t MaxLogSize() {
//   return (GET_ROO_FLAG(roo_logging_max_log_size) > 0
//               ? GET_ROO_FLAG(roo_logging_max_log_size)
//               : 1);
// }

// // Returns true if time going back has been detected, or if the wall clock
// // itself has changed.
// bool TimeHasChanged(roo_time::WallTime now) {
//   static WallTimeClockPtr clock = GET_ROO_FLAG(roo_logging_wall_time_clock);
//   static roo_time::WallTime last;
//   bool result = false;
//   WallTimeClockPtr current_clock = GET_ROO_FLAG(roo_logging_wall_time_clock);
//   if (current_clock != clock) {
//     clock = current_clock;
//     result = true;
//   }
//   if (now < last) {
//     result = true;
//   }
//   last = now;
//   return result;
// }

// class Logger {
//  public:
//   virtual ~Logger();

//   // Writes "message[0,message_len-1]" corresponding to an event that
//   // occurred at "timestamp".  If "force_flush" is true, the log file
//   // is flushed immediately.
//   //
//   // The input message has already been formatted as deemed
//   // appropriate by the higher level logging facility.  For example,
//   // textual log messages already contain timestamps, and the
//   // file:linenumber header.
//   virtual void Write(bool force_flush, roo_time::Uptime uptime,
//                      roo_time::WallTime walltime, const char* message,
//                      int message_len) = 0;

//   // Flush any buffered messages
//   virtual void Flush() = 0;

//   // Get the current LOG file size.
//   // The returned value is approximate since some
//   // logged data may not have been flushed to disk yet.
//   virtual uint32_t LogSize() = 0;
// };

// // Encapsulates all file-system related state
// class LogFileObject : public Logger {
//  public:
//   LogFileObject(LogSeverity severity, const char* base_filename)
//       : base_filename_selected_(base_filename != NULL),
//         base_filename_((base_filename != NULL) ? base_filename : ""),
//         // symlink_basename_(
//         //     glog_internal_namespace_::ProgramInvocationShortName()),
//         // filename_extension_(),
//         file_(NULL),
//         severity_(severity),
//         bytes_since_flush_(0),
//         dropped_mem_length_(0),
//         file_length_(0),
//         rollover_attempt_(kRolloverAttemptFrequency - 1),
//         next_flush_time_() {
//     assert(severity >= 0);
//     assert(severity < NUM_SEVERITIES);
//   }

//   ~LogFileObject();

//   void Write(bool force_flush, roo_time::Uptime uptime,
//              roo_time::WallTime walltime, const char* message,
//              int message_len) override {
//     // We don't log if the base_name_ is "" (which means "don't write")
//     if (base_filename_selected_ && base_filename_.isEmpty()) {
//       return;
//     }

//     if (static_cast<int>(file_length_ >> 20) >= MaxLogSize() ||
//         TimeHasChanged(walltime)) {
//       if (file_ != NULL) fclose(file_);
//       file_ = NULL;
//       file_length_ = bytes_since_flush_ = dropped_mem_length_ = 0;
//       rollover_attempt_ = kRolloverAttemptFrequency - 1;
//     }

//     // If there's no destination file, make one before outputting
//     if (file_ == NULL) {
//       // Try to rollover the log file every 32 log messages.  The only time
//       // this could matter would be when we have trouble creating the log
//       // file.  If that happens, we'll lose lots of log messages, of course!
//       if (++rollover_attempt_ != kRolloverAttemptFrequency) return;
//       rollover_attempt_ = 0;

//       struct ::tm tm_time;
//       localtime_r(&timestamp, &tm_time);

//       // The logfile's filename will have the date/time & pid in it
//       ostringstream time_pid_stream;
//       time_pid_stream.fill('0');
//       time_pid_stream << 1900 + tm_time.tm_year << setw(2) << 1 + tm_time.tm_mon
//                       << setw(2) << tm_time.tm_mday << '-' << setw(2)
//                       << tm_time.tm_hour << setw(2) << tm_time.tm_min << setw(2)
//                       << tm_time.tm_sec << '.' << GetMainThreadPid();
//       const string& time_pid_string = time_pid_stream.str();

//       if (base_filename_selected_) {
//         if (!CreateLogfile(time_pid_string)) {
//           perror("Could not create log file");
//           fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n",
//                   time_pid_string.c_str());
//           return;
//         }
//       } else {
// #if defined(OS_EMBEDDED)
//         string stripped_filename = "log.";
//         stripped_filename =
//             stripped_filename + LogSeverityNames[severity_] + '.';
// #else
//         // If no base filename for logs of this severity has been set, use a
//         // default base filename of
//         // "<program name>.<hostname>.<user name>.log.<severity level>.".  So
//         // logfiles will have names like
//         // webserver.examplehost.root.log.INFO.19990817-150000.4354, where
//         // 19990817 is a date (1999 August 17), 150000 is a time (15:00:00),
//         // and 4354 is the pid of the logging process.  The date & time reflect
//         // when the file was created for output.
//         //
//         // Where does the file get put?  Successively try the directories
//         // "/tmp", and "."
//         string stripped_filename(
//             glog_internal_namespace_::ProgramInvocationShortName());
//         string hostname;
//         GetHostName(&hostname);

//         stripped_filename = stripped_filename + '.' + hostname + ".log." +
//                             LogSeverityNames[severity_] + '.';
// #endif

//         // We're going to (potentially) try to put logs in several different
//         // dirs
//         const vector<string>& log_dirs = GetLoggingDirectories();

//         // Go through the list of dirs, and try to create the log file in each
//         // until we succeed or run out of options
//         bool success = false;
//         for (vector<string>::const_iterator dir = log_dirs.begin();
//              dir != log_dirs.end(); ++dir) {
//           base_filename_ = *dir + "/" + stripped_filename;
//           if (CreateLogfile(time_pid_string)) {
//             success = true;
//             break;
//           }
//         }
//         // If we never succeeded, we have to give up
//         if (success == false) {
//           perror("Could not create logging file");
//           fprintf(stderr, "COULD NOT CREATE A LOGGINGFILE %s!",
//                   time_pid_string.c_str());
//           return;
//         }
//       }

//       // Write a header message into the log file
//       ostringstream file_header_stream;
//       file_header_stream.fill('0');
//       file_header_stream << "Log file created at: " << 1900 + tm_time.tm_year
//                          << '/' << setw(2) << 1 + tm_time.tm_mon << '/'
//                          << setw(2) << tm_time.tm_mday << ' ' << setw(2)
//                          << tm_time.tm_hour << ':' << setw(2) << tm_time.tm_min
//                          << ':' << setw(2) << tm_time.tm_sec << '\n'
//                          << "Running on machine: " << LogDestination::hostname()
//                          << '\n'
//                          << "Log line format: [IWEF]mmdd hh:mm:ss.uuuuuu "
//                          << "threadid file:line] msg" << '\n';
//       const string& file_header_string = file_header_stream.str();

//       const int header_len = file_header_string.size();
//       fwrite(file_header_string.data(), 1, header_len, file_);
//       file_length_ += header_len;
//       bytes_since_flush_ += header_len;
//     }

//     // Write to LOG file
//     if (!stop_writing) {
//       // fwrite() doesn't return an error when the disk is full, for
//       // messages that are less than 4096 bytes. When the disk is full,
//       // it returns the message length for messages that are less than
//       // 4096 bytes. fwrite() returns 4096 for message lengths that are
//       // greater than 4096, thereby indicating an error.
//       errno = 0;
//       fwrite(message, 1, message_len, file_);
//       if (GET_ROO_GLOG_FLAG(stop_logging_if_full_disk) &&
//           errno == ENOSPC) {  // disk full, stop writing to disk
//         stop_writing = true;  // until the disk is
//         return;
//       } else {
//         file_length_ += message_len;
//         bytes_since_flush_ += message_len;
//       }
//     } else {
//       if (CycleClock_Now() >= next_flush_time_)
//         stop_writing = false;  // check to see if disk has free space.
//       return;                  // no need to flush
//     }

// #if defined(OS_EMBEDDED)
// #define MAX_BYTES_TO_BUFFER 2000
// #else
// #define MAX_BYTES_TO_BUFFER 1000000
// #endif

//     // See important msgs *now*.  Also, flush logs at least every 10^6 chars,
//     // or every "FLAGS_logbufsecs" seconds.
//     if (force_flush || (bytes_since_flush_ >= MAX_BYTES_TO_BUFFER) ||
//         (CycleClock_Now() >= next_flush_time_)) {
//       FlushUnlocked();
// #ifdef OS_LINUX
//       // Only consider files >= 3MiB
//       if (GET_ROO_GLOG_FLAG(drop_log_memory) && file_length_ >= (3 << 20)) {
//         // Don't evict the most recent 1-2MiB so as not to impact a tailer
//         // of the log file and to avoid page rounding issue on linux < 4.7
//         uint32_t total_drop_length =
//             (file_length_ & ~((1 << 20) - 1)) - (1 << 20);
//         uint32_t this_drop_length = total_drop_length - dropped_mem_length_;
//         if (this_drop_length >= (2 << 20)) {
//           // Only advise when >= 2MiB to drop
//           posix_fadvise(fileno(file_), dropped_mem_length_, this_drop_length,
//                         POSIX_FADV_DONTNEED);
//           dropped_mem_length_ = total_drop_length;
//         }
//       }
// #endif
//     }
//   }

//   // // Configuration options
//   // void SetBasename(const char* basename);
//   // void SetExtension(const char* ext);
//   // void SetSymlinkBasename(const char* symlink_basename);

//   // Normal flushing routine
//   void Flush() override;

//   // It is the actual file length for the system loggers,
//   // i.e., INFO, ERROR, etc.
//   uint32_t LogSize() override {
//     // MutexLock l(&lock_);
//     return file_length_;
//   }

//  private:
//   // static const uint32_t kRolloverAttemptFrequency = 0x20;

//   // Mutex lock_;
//   bool base_filename_selected_;
//   String base_filename_;
//   // string symlink_basename_;
//   // string filename_extension_;     // option users can specify (eg to add
//   // port#)
//   FILE* file_;
//   LogSeverity severity_;
//   uint32_t bytes_since_flush_;
//   uint32_t dropped_mem_length_;
//   uint32_t file_length_;
//   unsigned int rollover_attempt_;
//   roo_time::Uptime next_flush_time_;

//   // Actually create a logfile using the value of base_filename_ and the
//   // supplied argument time_pid_string
//   // REQUIRES: lock_ is held
//   bool CreateLogfile(const String& time_pid_string);
// };

// class LogDestination {
//  public:
//   //   friend void ReprintFatalMessage();
//   //   friend base::Logger* base::GetLogger(LogSeverity);
//   //   friend void base::SetLogger(LogSeverity, base::Logger*);

//  private:
//   LogDestination(LogSeverity severity, const char* base_filename)
//       : fileobject_(severity, base_filename), logger_(&fileobject_) {}

//   ~LogDestination() = default;

//   LogFileObject fileobject_;
//   Logger* logger_;  // Either &fileobject_, or wrapper around it
// };

// LogDestination* log_destinations_[NUM_SEVERITIES];

// LogDestination* LogDestination::log_destination(LogSeverity severity) {
//   //   assert(severity >=0 && severity < NUM_SEVERITIES);
//   if (!log_destinations_[severity]) {
//     log_destinations_[severity] = new LogDestination(severity, nullptr);
//   }
//   return log_destinations_[severity];
// }

// void MaybeLogToLogfile(LogSeverity severity, time_t timestamp,
//                        const char* message, size_t len) {
//   const bool should_flush = severity > GET_ROO_FLAG(roo_logging_logbuflevel);
//   LogDestination* destination = log_destination(severity);
//   destination->logger_->Write(should_flush, timestamp, message, len);
// }

// }  // namespace

// void LogToAllLogfiles(LogSeverity severity, time_t timestamp,
//                       const char* message, size_t len) {
//   if (GET_ROO_FLAG(roo_logging_alsologtologfiles)) {
//     for (int i = severity; i >= 0; --i) {
//       MaybeLogToLogfile(i, timestamp, message, len);
//     }
//   }
// }

// }  // namespace roo_logging
