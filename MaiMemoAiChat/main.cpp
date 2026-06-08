#include "mainwindow.h"
#include "AppCoordinator.h"
#include "IAppCoordinator.h"
#include "Hold/Hold.h"
#include "LogManage.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto env = QProcessEnvironment::systemEnvironment();
    QString apiKey = env.value(QStringLiteral("DASHSCOPE_API_KEY"));

    if (apiKey.isEmpty()) {
        qWarning() << "DASHSCOPE_API_KEY env var not set. AI features will not work.";
        qWarning() << "Set it via: set DASHSCOPE_API_KEY=your-key";
    }

    // 数据目录放在可执行文件同级
    QString dataDir = QFileInfo(QCoreApplication::applicationFilePath())
                          .absolutePath() + QStringLiteral("/data");
    QDir().mkpath(dataDir);

    // 初始化日志系统（在 AppCoordinator 之前，确保启动期日志也能被捕获）
    QString appDir = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
    QString logDir = appDir + QStringLiteral("/logs");
    Hold logHold(dataDir);
    LogManage::init(&logHold, logDir);

    SPDLOG_DEBUG("********************结合ai与间隔重复的英语学习软件--启动**************");

    AppConfig config;
    config.dataDir = dataDir;
    config.srsConfig.deckName = QStringLiteral("English");
    config.srsConfig.modelName = QStringLiteral("Basic");
    config.defaultSceneType = SceneType::Reading;
    config.dueEntryLimit = 20;

    config.sceneConfig.aiProvider = AiProvider::QianWen;
    config.sceneConfig.apiKey = apiKey;
    config.sceneConfig.model = QStringLiteral("qwen-max");
    config.sceneConfig.temperature = 0.7f;
    config.sceneConfig.maxTokens = 2048;

    auto *coordinator = new AppCoordinator(&a);
    auto *window = new MainWindow;

    coordinator->initialize(window, window, config);

    window->show();
    return QCoreApplication::exec();
}
