
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
        "//lib/roo_flags",
        "//lib/roo_time",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32",
    ]
)
