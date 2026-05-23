#include "mainwindow.h"
#include "AppCoordinator.h"
#include "IAppCoordinator.h"

#include <QApplication>
#include <QDir>
#include <QProcessEnvironment>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 构建应用配置
    AppConfig config;
    config.dataDir = QStringLiteral("./data");
    config.srsConfig.deckName = QStringLiteral("English");
    config.srsConfig.modelName = QStringLiteral("Basic");
    config.defaultSceneType = SceneType::Reading;
    config.dueEntryLimit = 20;

    // AI API key 从环境变量读取
    auto env = QProcessEnvironment::systemEnvironment();
    config.sceneConfig.aiProvider = AiProvider::QianWen;
    config.sceneConfig.apiKey = env.value(QStringLiteral("DASHSCOPE_API_KEY"),
                                          QStringLiteral("sk-your-api-key"));
    config.sceneConfig.model = QStringLiteral("qwen-max");
    config.sceneConfig.temperature = 0.7f;
    config.sceneConfig.maxTokens = 2048;

    // 确保数据目录存在
    QDir().mkpath(config.dataDir);

    // 创建模块并串联
    auto *coordinator = new AppCoordinator(&a);
    auto *window = new MainWindow;

    // initialize(QObject* for signals, IUIModule* for method calls)
    coordinator->initialize(window, window, config);

    window->show();
    return QCoreApplication::exec();
}
