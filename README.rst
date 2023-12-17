roo_logging Library
======================

roo_logging is a lightweight C++14 library that implements application-level logging for
Arduino systems. The library provides logging APIs based on C++-style streams and
various helper macros.

The library will add only ~6 KB to your binary size. Debug-only logging can be completely
optimized away in release builds.

Getting Started
---------------

You can log a message by simply streaming things to ``LOG``\ (<a
particular `severity level <#severity-levels>`__>), e.g.,

.. code:: cpp

   #include <roo_logging.h>

   void loop() {

       // ...
       LOG(INFO) << "Found " << num_cookies << " cookies";
   }

.. contents:: Table of Contents



User Guide
----------

roo_logging defines a series of macros that simplify many common logging tasks.
You can log messages by severity level, control logging behavior via flags,
log based on conditionals, abort the program when
expected conditions are not met, introduce your own verbose logging
levels, customize the prefix attached to log messages, and more.

Following sections describe the functionality supported by roo_logging. Please note
this description may not be complete but limited to the most useful ones. If you
want to find less common features, please check header files under `src/roo_logging
<src/roo_logging>`__ directory.

Severity Levels
~~~~~~~~~~~~~~~

You can specify one of the following severity levels (in increasing
order of severity): ``INFO``, ``WARNING``, ``ERROR``, and ``FATAL``.
Logging a ``FATAL`` message terminates the program (after the message is
logged). Note that messages of a given severity are logged not only in
the logfile for that severity, but also in all logfiles of lower
severity. E.g., a message of severity ``FATAL`` will be logged to the
logfiles of severity ``FATAL``, ``ERROR``, ``WARNING``, and ``INFO``.

The ``DFATAL`` severity logs a ``FATAL`` error in debug mode (i.e.,
there is no ``NDEBUG`` macro defined), but avoids halting the program in
production by automatically reducing the severity to ``ERROR``.

By default, roo_logging writes to the default Serial interface.

Setting Flags
~~~~~~~~~~~~~

Several flags influence roo_logging's output behavior.
The following flags are most commonly used:

``roo_logging_minloglevel`` (``int``, default=0, which is ``INFO``)
   Log messages at or above this level. Again, the numbers of severity
   levels ``INFO``, ``WARNING``, ``ERROR``, and ``FATAL`` are 0, 1, 2,
   and 3, respectively.

``roo_logging_wall_time_clock`` (``roo_time::WallTimeClock*``, default=nullptr)
   Use the specified wall time clock to determine absolute time. If nullptr,
   logs using relative time (i.e. time since program start).

``roo_logging_timezone`` (``roo_time::TimeZone``, default=roo_time::UTC)
   Use the specified time zone when reporting time in log messages. Ignored
   if ``roo_logging_wall_time_clock`` is nullptr.

``roo_logging_colorlogtostderr`` (``bool``, default=false)
   If set, uses ANSI sequences to color log messages sent to Serial.
   You need to configure the console to recognize color input
   (e.g., if using PlatformIO, set ``monitor_raw = yes`` in ``platformio.ini``.)

You can set flag values in your program, using the ``SET_ROO_GLOG(flag_name, value)``
macro. Most settings start working immediately after the update.

Conditional / Occasional Logging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sometimes, you may only want to log a message under certain conditions.
You can use the following macros to perform conditional logging:

.. code:: cpp

   LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";

The "Got lots of cookies" message is logged only when the variable
``num_cookies`` exceeds 10. If a line of code is executed many times, it
may be useful to only log a message at certain intervals. This kind of
logging is most useful for informational messages.

.. code:: cpp

   LOG_EVERY_N(INFO, 10) << "Got the " << roo_logging::COUNTER << "th cookie";

The above line outputs a log messages on the 1st, 11th, 21st, ... times
it is executed. Note that the special ``google::COUNTER`` value is used
to identify which repetition is happening.

You can combine conditional and occasional logging with the following
macro.

.. code:: cpp

   LOG_IF_EVERY_N(INFO, (size > 1024), 10) << "Got the " << roo_logging::COUNTER
                                           << "th big cookie";

Instead of outputting a message every nth time, you can also limit the
output to the first n occurrences:

.. code:: cpp

   LOG_FIRST_N(INFO, 20) << "Got the " << roo_logging::COUNTER << "th cookie";

Outputs log messages for the first 20 times it is executed. Again, the
``roo_logging::COUNTER`` identifier indicates which repetition is happening.

Other times, it is desired to only log a message periodically based on a time.
So for example, to log a message every 10ms:

.. code:: cpp

   LOG_EVERY_T(INFO, 0.01) << "Got a cookie";

Or every 2.35s:

.. code:: cpp

   LOG_EVERY_T(INFO, 2.35) << "Got a cookie";

Debug Mode Support
~~~~~~~~~~~~~~~~~~

Special "debug mode" logging macros only have an effect in debug mode
and are compiled away to nothing for non-debug mode compiles. Use these
macros to avoid slowing down your production application due to
excessive logging.

.. code:: cpp

   DLOG(INFO) << "Found cookies";
   DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
   DLOG_EVERY_N(INFO, 10) << "Got the " << roo_logging::COUNTER << "th cookie";

To disable debug mode, define NDEBUG macro. For example, if using PlatformIO,
add ``build_flags = -DNDEBUG`` to your configuration.

``CHECK`` Macros
~~~~~~~~~~~~~~~~

It is a good practice to check expected conditions in your program
frequently to detect errors as early as possible. The ``CHECK`` macro
provides the ability to abort the application when a condition is not
met, similar to the ``assert`` macro defined in the standard C library.

``CHECK`` aborts the application if a condition is not true. Unlike
``assert``, it is \*not\* controlled by ``NDEBUG``, so the check will be
executed regardless of compilation mode. Therefore, ``fp->Write(x)`` in
the following example is always executed:

.. code:: cpp

   CHECK(fp->Write(x) == 4) << "Write failed!";

There are various helper macros for equality/inequality checks -
``CHECK_EQ``, ``CHECK_NE``, ``CHECK_LE``, ``CHECK_LT``, ``CHECK_GE``,
and ``CHECK_GT``. They compare two values, and log a ``FATAL`` message
including the two values when the result is not as expected. The values
must have :cpp:`operator<<(roo_logging::Stream&, ...)` defined.

You may append to the error message like so:

.. code:: cpp

   CHECK_NE(1, 2) << ": The world must be ending!";

We are very careful to ensure that each argument is evaluated exactly
once, and that anything which is legal to pass as a function argument is
legal here. In particular, the arguments may be temporary expressions
which will end up being destroyed at the end of the apparent statement,
for example:

.. code:: cpp

   CHECK_EQ(String("abc")[1], ’b’);

The compiler reports an error if one of the arguments is a pointer and the other
is :cpp:`nullptr`. To work around this, simply :cpp:`static_cast` :cpp:`nullptr` to
the type of the desired pointer.

.. code:: cpp

   CHECK_EQ(some_ptr, static_cast<SomeType*>(nullptr));

Better yet, use the ``CHECK_NOTNULL`` macro:

.. code:: cpp

   CHECK_NOTNULL(some_ptr);
   some_ptr->DoSomething();

Since this macro returns the given pointer, this is very useful in
constructor initializer lists.

.. code:: cpp

   struct S {
       S(Something* ptr) : ptr_(CHECK_NOTNULL(ptr)) {}
       Something* ptr_;
   };

Note that you cannot use this macro as a C++ stream due to this feature.
Please use ``CHECK_EQ`` described above to log a custom message before
aborting the application.

If you are comparing C strings (:cpp:`char *`), a handy set of macros performs
case sensitive as well as case insensitive comparisons - ``CHECK_STREQ``,
``CHECK_STRNE``, ``CHECK_STRCASEEQ``, and ``CHECK_STRCASENE``. The CASE versions
are case-insensitive. You can safely pass :cpp:`nullptr` pointers for this macro. They
treat :cpp:`nullptr` and any non-:cpp:`nullptr` string as not equal. Two :cpp:`nullptr`\
s are equal.

Note that both arguments may be temporary strings which are destructed
at the end of the current "full expression" (e.g.,
:cpp:`CHECK_STREQ(Foo().c_str(), Bar().c_str())` where ``Foo`` and ``Bar``
return C++’s :cpp:`std::string`).

The ``CHECK_DOUBLE_EQ`` macro checks the equality of two floating point
values, accepting a small error margin. ``CHECK_NEAR`` accepts a third
floating point argument, which specifies the acceptable error margin.


Performance of Messages
~~~~~~~~~~~~~~~~~~~~~~~

The conditional logging macros provided by roo_logging (e.g., ``CHECK``,
``LOG_IF``, etc.) are carefully implemented and don’t execute
the right hand side expressions when the conditions are false. So, the
following check may not sacrifice the performance of your application.

.. code:: cpp

   CHECK(obj.ok) << obj.CreatePrettyFormattedStringButVerySlow();

User-defined Failure Function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``FATAL`` severity level messages or unsatisfied ``CHECK`` condition
terminate your program. You can change the behavior of the termination
by :cpp:`InstallFailureFunction`.

.. code:: cpp

   void YourFailureFunction() {
     // Reports something...
     exit(EXIT_FAILURE);
   }

   void setup() {
     roo_logging::InstallFailureFunction(&YourFailureFunction);
   }

By default, roo_logging tries to dump stacktrace and makes the program exit
with status 1. The stacktrace is produced only when you run the program
on an architecture for which roo_logging supports stack tracing (as of
September 2008, roo_logging supports stack tracing for ESP32 and Linux).


Roo-Logging-Style ``perror()``
~~~~~~~~~~~~~~~~~~~~~~~~~

``PLOG()`` and ``PLOG_IF()`` and ``PCHECK()`` behave exactly like their
``LOG*`` and ``CHECK`` equivalents with the addition that they append a
description of the current state of errno to their output lines. E.g.

.. code:: cpp

   PCHECK(write(1, nullptr, 2) >= 0) << "Write nullptr failed";

This check fails with the following error message.

::

   F0825 185142 test.cc:22] Check failed: write(1, nullptr, 2) >= 0 Write nullptr failed: Bad address [14]


Strip Logging Messages
~~~~~~~~~~~~~~~~~~~~~~

Strings used in log messages can increase the size of your binary and
present a privacy concern. You can therefore instruct roo_logging to remove all
strings which fall below a certain severity level by using the
``ROO_LOGGING_STRIP_LOG`` macro:

If your application has code like this:

.. code:: cpp

   #define ROO_LOGGING_STRIP_LOG 1    // this must go before the #include!
   #include <roo_logging.h>

The compiler will remove the log messages whose severities are less than
the specified integer value.
