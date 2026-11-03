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
    copts = [
        "-fno-exceptions",
        "-fno-rtti",
    ],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        "@roo_backport",
        "@roo_flags",
        "@roo_threads",
        "@roo_time",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_arduino": [
            "@roo_testing//:arduino",
        ],
        "@roo_testing//roo_testing/platforms:is_idf": [
            "@roo_testing//roo_testing/frameworks/esp-idf",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "roo_logging_test",
    size = "small",
    srcs = [
        "test/buffered_log_stream_test.cpp",
        "test/check_message_test.cpp",
        "test/roo_logging_test.cpp",
        "test/time_format_test.cpp",
    ],
    copts = [
        "-Iexternal/gtest/include",
        "-fno-rtti",
    ],
    includes = ["src"],
    linkstatic = 1,
    target_compatible_with = [
        "@roo_testing//roo_testing/platforms:arduino",
    ],
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
        "test/buffered_log_stream_test.cpp",
        "test/check_message_test.cpp",
        "test/roo_logging_test.cpp",
        "test/time_format_test.cpp",
    ],
    copts = [
        "-Iexternal/gtest/include",
        "-fno-rtti",
    ],
    includes = ["src"],
    linkstatic = 1,
    target_compatible_with = select({
        "@roo_testing//roo_testing/platforms:is_arduino": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":roo_logging",
        "@googletest//:gtest_main",
    ],
)
