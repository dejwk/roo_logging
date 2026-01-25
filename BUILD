load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "roo_logging",
    srcs = glob(
        [
            "src/**/*.cpp",
            "src/**/*.h",
        ],
        exclude = ["test/**"],
    ),
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        "@roo_backport",
        "@roo_flags",
        "@roo_testing//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32",
        "@roo_threads",
        "@roo_time",
    ],
)

cc_test(
    name = "roo_logging_test",
    size = "small",
    srcs = [
        "test/roo_logging_test.cpp",
    ],
    copts = ["-Iexternal/gtest/include"],
    includes = ["src"],
    linkstatic = 1,
    deps = [
        ":roo_logging",
        "@roo_testing//:arduino_gtest_main",
    ],
)

# Same as above, but linked against the regular gtest for Linux; not emulating Arduino.
cc_test(
    name = "roo_logging_linux_test",
    size = "small",
    srcs = [
        "test/roo_logging_test.cpp",
    ],
    copts = ["-Iexternal/gtest/include"],
    includes = ["src"],
    linkstatic = 1,
    deps = [
        ":roo_logging",
        "@googletest//:gtest_main",
    ],
)
