add_library(DataSync STATIC
    MemEntry.h
    ISRSEngine.h
    AnkiConnectEngine.h
    AnkiConnectEngine.cpp
    IDataSync.h
    DataSync.h
    DataSync.cpp
)

target_include_directories(DataSync PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(DataSync PUBLIC Qt::Core Qt::Network Hold)
