add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(spdlog INTERFACE SPDLOG_HEADER_ONLY)
