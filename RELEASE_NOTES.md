# roo_logging 1.5.11

- Upgrade `roo_time` from 2.0.0 to 2.0.1 and `roo_threads` from 1.2.8 to 1.2.9 in Bazel and PlatformIO dependencies.
- Upgrade `roo_testing` from 2.1.2 to 2.3.0.
- Automatically select the ESP-IDF frontend for Bazel `run` commands targeting ESP-IDF examples when no frontend is explicitly configured.

---

# roo_logging 1.5.10

- Updated Bazel and PlatformIO dependencies to `roo_time` 2.0.0, `roo_backport` 1.2.4, `roo_flags` 1.2.5, and `roo_threads` 1.2.8.
- Updated build and test dependencies to `rules_cc` 0.2.25, GoogleTest 1.18.0.bcr.1, and `roo_testing` 2.1.2; refreshed CI to match.
- Added consolidated release notes for previous versions.

---

# [roo_logging 1.5.9](https://github.com/dejwk/roo_logging/releases/tag/1.5.9)

Published 2026-09-19.

* Updated roo_time dependency to 1.5.0 and migrated wall-time logging to its new UtcOffset API. The roo_logging_timezone flag now uses roo_time::UtcOffset; update existing configurations accordingly.
* Fix for the compile error with newer esp-idf toolchains (PIO platform = espressif32@6.13.0, framework = espidf). Thanks Gamadril for the contribution.


---

# [roo_logging 1.5.8](https://github.com/dejwk/roo_logging/releases/tag/1.5.8)

Published 2026-08-29.

This release modernizes host-emulation support and makes every Arduino example runnable with Bazel.

- Added runnable host-emulator targets for all examples.
- Added an emulated ESP32 Wi‑Fi environment for the `walltime` NTP example.
- Added documentation for Arduino and ESP-IDF host-emulation workflows.
- Updated CI to use `roo_testing` 2.x profiles and refreshed AddressSanitizer configuration.
- Improved framework-specific build configuration, including correct Arduino stream guards.
- Updated dependencies: `roo_backport` 1.2.3, `roo_flags` 1.2.4, `roo_time` 1.4.7, and `roo_threads` 1.2.7.

No API-breaking changes.

---

# [roo_logging 1.5.7](https://github.com/dejwk/roo_logging/releases/tag/1.5.7)

Published 2026-03-25.

* Adding log override for nullptr_t, to support CHECK_EQ(p, nullptr).

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.5.6...1.5.7

---

# [roo_logging 1.5.6](https://github.com/dejwk/roo_logging/releases/tag/1.5.6)

Published 2026-03-18.

Fixing compilation issues on ESP8266.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.5.5...1.5.6

---

# [roo_logging 1.5.5](https://github.com/dejwk/roo_logging/releases/tag/1.5.5)

Published 2026-02-25.

* Updated dependencies, thus fixing some issues in tests.
* Updated docs to the doxygen format.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.5.4...1.5.5

---

# [roo_logging 1.5.4](https://github.com/dejwk/roo_logging/releases/tag/1.5.4)

Published 2026-02-24.

* Updated dependencies.
* Cleaned up compilation warnings.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.5.3...1.5.4

---

# [roo_logging 1.5.3](https://github.com/dejwk/roo_logging/releases/tag/1.5.3)

Published 2026-01-25.

* Fixing pesky compilation issues with isp-idf in some cases,
* Fixing unit tests, broken by the recent bazel release.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.5.2...1.5.3

---

# [roo_logging 1.5.2](https://github.com/dejwk/roo_logging/releases/tag/1.5.2)

Published 2026-01-06.

Fixing a dreadful compilation error that crept up to the latest release.

---

# [roo_logging 1.5.1](https://github.com/dejwk/roo_logging/releases/tag/1.5.1)

Published 2026-01-06.

Fixed unit testing issues under raw gtest (not Arduino emulated).

---

# [roo_logging 1.5.0](https://github.com/dejwk/roo_logging/releases/tag/1.5.0)

Published 2026-01-06.

* Added support for esp-idf,
* Added support for RP2040 Raspberry Pi Pico SMP (with FreeRTOS),
* Fixing logging very early (on ESP32 you can now log from static initializers),
* Adding optional logging of core ID.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.4.3...1.5.0

---

# [roo_logging 1.4.3](https://github.com/dejwk/roo_logging/releases/tag/1.4.3)

Published 2025-11-12.

Fixing logging with JTAG.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.4.2...1.4.3

---

# [roo_logging 1.4.2](https://github.com/dejwk/roo_logging/releases/tag/1.4.2)

Published 2025-10-30.

* Bugfix: made logging work even if called from static initializer. You can now see logging in the first millisecond from controller start, way before setup() and Serial.begin().
* Better CI, and .gitignore.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.4.0...1.4.2

---

# [roo_logging 1.4.0](https://github.com/dejwk/roo_logging/releases/tag/1.4.0)

Published 2025-10-14.

* More memory-efficient logging on ESP32,
* Switching to the new roo_time API,
* Enabling basic configurability via compiler macros.

---

# [roo_logging 1.3.0](https://github.com/dejwk/roo_logging/releases/tag/1.3.0)

Published 2025-10-05.

* Making logging thread-safe.
* Better log messages under roo_testing emulator.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.2.0...1.3.0

---

# [roo_logging 1.2.0](https://github.com/dejwk/roo_logging/releases/tag/1.2.0)

Published 2025-08-09.

Making roo_logging a Bazel module, so that it can be used in unit tests of depending libraries.

Also, a small tweak when running under the Linux emulator (adding thread name to the log line).

Added unit tests, and CI/CD to have them run on submit.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.1.0...1.2.0

---

# [roo_logging 1.1.0](https://github.com/dejwk/roo_logging/releases/tag/1.1.0)

Published 2025-07-04.

* Improvements for ESP32:
   * Printing thread (task) name in the log line,
   * More conservative stack usage.
* Fixing compilation issues under Arduino IDE.
* Updated demangler on Linux. 

---

# [roo_logging 1.0.6](https://github.com/dejwk/roo_logging/releases/tag/1.0.6)

Published 2024-12-29.

* Made compatible with with ESP32C3.
* Fixed logging of elapsed time.


---

# [roo_logging 1.0.5](https://github.com/dejwk/roo_logging/releases/tag/1.0.5)

Published 2024-08-08.

* Adding support for logging iterable collections.
* Fixing logging of flating point values.
* Adding logging for time intervals.
* Slightly changing the logging format of uptime, to be more standard.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.0.4...1.0.5

---

# [roo_logging 1.0.4](https://github.com/dejwk/roo_logging/releases/tag/1.0.4)

Published 2024-08-06.

Added support for verbose and conditionally-enabled module logging, useful for implementing debug logging in libraries.

---

# [roo_logging 1.0.3](https://github.com/dejwk/roo_logging/releases/tag/1.0.3)

Published 2024-01-03.

Improved templates for logging strings. Works for std::string, std::string_view, and roo_display::StringView, without actually depending on them.

---

# [roo_logging 1.0.2](https://github.com/dejwk/roo_logging/releases/tag/1.0.2)

Published 2023-12-23.

Fix: logging floating-point values now possible.

---

# [roo_logging 1.0.1](https://github.com/dejwk/roo_logging/releases/tag/1.0.1)

Published 2023-12-18.

Bug fixes; added support for logging pointers.

**Full Changelog**: https://github.com/dejwk/roo_logging/compare/1.0.0...1.0.1

---

# [roo_logging 1.0.0](https://github.com/dejwk/roo_logging/releases/tag/1.0.0)

Published 2023-12-17.

Initial release.

---

