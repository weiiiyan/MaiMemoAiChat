add_library(SceneOrchestrator STATIC
    SceneTypes.h
    IAIManager.h
    WenXinAiManager.h
    WenXinAiManager.cpp
    QianWenAiManager.h
    QianWenAiManager.cpp
    ISceneOrchestrator.h
    SceneOrchestrator.h
    SceneOrchestrator.cpp
)

target_include_directories(SceneOrchestrator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SceneOrchestrator PUBLIC Qt::Core Qt::Network DataSync)
