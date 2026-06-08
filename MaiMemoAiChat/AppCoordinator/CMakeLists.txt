add_library(AppCoordinator STATIC
    IUIModule.h
    IAppCoordinator.h
    AppCoordinator.h
    AppCoordinator.cpp
)

target_include_directories(AppCoordinator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(AppCoordinator
    PUBLIC Qt::Core
    PRIVATE Hold DataSync SceneOrchestrator spdlog
)
