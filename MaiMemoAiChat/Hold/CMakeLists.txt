add_library(Hold STATIC
    Hold.h
    Hold.cpp
)

target_include_directories(Hold PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Hold PUBLIC Qt::Core)
