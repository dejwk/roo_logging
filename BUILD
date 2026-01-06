
cc_library(
    name = "roo_logging",
    visibility = ["//visibility:public"],
    srcs = glob(
        [
            "src/**/*.cpp",
            "src/**/*.h",
        ],
        exclude = ["test/**"],
    ),
    includes = [ "src" ],
    deps = [
        "@roo_backport",
        "@roo_flags",
        "@roo_time",
        "@roo_threads",
        "@roo_testing//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32",
    ]
)

cc_test(
    name = "roo_logging_test",
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
    size = "small",
)

# Same as above, but linked against the regular gtest for Linux; not emulating Arduino.
cc_test(
    name = "roo_logging_linux_test",
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
    size = "small",
)
