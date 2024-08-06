#include <Arduino.h>
#include <roo_logging.h>

// Per-module logging is intended for use as debug logging in libraries.
//
// To allow users to conditionally enable verbose logging in your library,
// use MLOG macros with appropriate module names. In simple cases, just use
// your library name as the module name. In more complex cases, you may want to
// define multiple modules per your library. It is recommended that you use your
// library name as the prefix of all these module names.
//
// In this example, we pretend to have a library with two levels of verbose
// logging, represented by two modules. The macro definitions are set up to make
// these modules hirerarchical; i.e. enabling the more verbose logging also
// enables logs at the lower verbosity.
//
// Note: generally, do not use per-module logging to report errors or warnings
// that signal unexpected conditions. Just use LOG(ERROR) and LOG(WARNING)
// directly.

// Experiment by commenting/uncommenting one or both of these lines.
//
// Note: normally these definitions would go into the command-line invocation of
// your compiler. For example, if using PlatformIO, you'd add them to
// build_flags in platformio.ini, e.g. as -DMLOG_my_library_detailed=1, etc.
#define MLOG_my_library_detailed 1
// #define MLOG_my_library 1

// These definitions implement the hierarchy of modules. In this case, enabling
// logging for my_library_detailed also enables logging for my_library.
#if defined(MLOG_my_library_detailed)
#define MLOG_my_library MLOG_my_library_detailed
#else
#define MLOG_my_library_detailed 0
#endif

#if !defined(MLOG_my_library)
#define MLOG_my_library 0
#endif

void setup() {}

int num_cookies = 0;

void loop() {
  MLOG(my_library) << "My library";
  MLOG(my_library_detailed) << "My library, more verbose logging";

#if MLOG_IS_ON(my_library_detailed)
  int size = random() % 2048;
  MLOG_IF_EVERY_N(my_library_detailed, (size > 1024), 10)
      << "Got the " << roo_logging::COUNTER << "th cookie, big one!";
#endif

  delay(500);
  ++num_cookies;
}
