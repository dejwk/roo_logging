cmake_minimum_required(VERSION 3.13...3.27)

if(NOT DEFINED PICO_SDK_PATH)
  set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
  message(FATAL_ERROR "Set PICO_SDK_PATH to a Raspberry Pi Pico SDK checkout.")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(roo_logging_pico LANGUAGES C CXX ASM)
pico_sdk_init()

set(ROO_LIBRARIES_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH
    "Directory containing roo_backport, roo_flags, roo_threads, and roo_time")
set(ROO_LOGGING_SRC "${CMAKE_CURRENT_LIST_DIR}/../../src")

foreach(library roo_backport roo_flags roo_threads roo_time)
  if(NOT EXISTS "${ROO_LIBRARIES_DIR}/${library}/src/${library}.h")
    message(FATAL_ERROR "Missing ${library}. Set ROO_LIBRARIES_DIR to its parent directory.")
  endif()
endforeach()

file(GLOB_RECURSE logging_sources CONFIGURE_DEPENDS "${ROO_LOGGING_SRC}/*.cpp")
file(GLOB_RECURSE time_sources CONFIGURE_DEPENDS "${ROO_LIBRARIES_DIR}/roo_time/src/*.cpp")

add_executable(roo_logging_pico main.cpp ${logging_sources} ${time_sources})
target_compile_features(roo_logging_pico PRIVATE cxx_std_14)
target_include_directories(roo_logging_pico PRIVATE
  "${ROO_LOGGING_SRC}"
  "${ROO_LIBRARIES_DIR}/roo_backport/src"
  "${ROO_LIBRARIES_DIR}/roo_flags/src"
  "${ROO_LIBRARIES_DIR}/roo_threads/src"
  "${ROO_LIBRARIES_DIR}/roo_time/src"
)
target_link_libraries(roo_logging_pico PRIVATE pico_stdlib)

if(NOT TARGET tinyusb_device)
  message(FATAL_ERROR "USB stdio requires TinyUSB. Initialize the Pico SDK submodules.")
endif()
pico_enable_stdio_usb(roo_logging_pico 1)
pico_enable_stdio_uart(roo_logging_pico 0)
pico_add_extra_outputs(roo_logging_pico)
