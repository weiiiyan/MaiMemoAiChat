#ifndef LOGMANAGE_H
#define LOGMANAGE_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <QByteArray>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QTextStream>

class Hold;

namespace LogManage {

/**
 * @brief 日志配置，对应 config.ini 内容。
 *
 * 默认值：3 个文件，每个 10 MB，文件与终端级别均为 debug。
 */
struct LogConfig {
    int     fileCount    = 3;
    int     maxFileSize  = 10 * 1024 * 1024;  // 字节，默认 10 MB
    QString fileLevel    = QStringLiteral("debug");
    QString consoleLevel = QStringLiteral("debug");
};

// ── 序列化 / 反序列化 ──

inline QByteArray serializeConfig(const LogConfig &cfg)
{
    QString out;
    QTextStream ts(&out);
    ts << "file_count="    << cfg.fileCount    << "\n"
       << "max_file_size=" << cfg.maxFileSize  << "\n"
       << "file_level="    << cfg.fileLevel    << "\n"
       << "console_level=" << cfg.consoleLevel << "\n";
    return out.toUtf8();
}

inline LogConfig parseConfig(const QByteArray &data)
{
    LogConfig cfg;
    const QString content = QString::fromUtf8(data);
    const QStringList lines = content.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        QString s = line.trimmed();
        if (s.isEmpty() || s.startsWith(QLatin1Char('#')))
            continue;
        int idx = s.indexOf(QLatin1Char('='));
        if (idx < 0)
            continue;
        const QString key = s.left(idx).trimmed();
        const QString val = s.mid(idx + 1).trimmed();
        if (key == QLatin1String("file_count")) {
            cfg.fileCount = val.toInt();
        } else if (key == QLatin1String("max_file_size")) {
            cfg.maxFileSize = val.toInt();
        } else if (key == QLatin1String("file_level")) {
            cfg.fileLevel = val;
        } else if (key == QLatin1String("console_level")) {
            cfg.consoleLevel = val;
        }
    }
    return cfg;
}

// ── 字符串 → spdlog 级别 ──

inline spdlog::level::level_enum toSpdLevel(const QString &s)
{
    // spdlog::level::from_str 接受 "trace"/"debug"/"info"/"warn"/"error"/"critical"/"off"
    return spdlog::level::from_str(s.toStdString());
}

// ── 初始化 ──

/**
 * @brief 初始化 spdlog 日志系统。
 *
 * 从 Hold 读取 config.ini 配置日志参数；若不存在则用默认值创建。
 * - 文件输出：rotating_file_sink（循环利用，最多 @c fileCount 个文件，每个 @c maxFileSize 字节）
 * - 终端输出：stdout_color_sink（带颜色）
 * - 日志格式：「时间][级别][线程][文件:行号][函数名] 消息内容」
 *
 * @param hold   用于读写 config.ini 的 Hold 实例
 * @param logDir 日志文件目录（如程序目录下的 logs 文件夹）
 */
inline void init(Hold *hold, const QString &logDir)
{
    LogConfig cfg;

    // 1. 尝试从 Hold 加载 config.ini
    static const QStringList kConfigId   = {QStringLiteral("config")};
    static const QString     kConfigName = QStringLiteral("config.ini");

    QByteArray raw = hold->load(kConfigId, kConfigName);
    if (!raw.isEmpty()) {
        cfg = parseConfig(raw);
    } else {
        // 写入默认配置，方便用户按需修改
        hold->save(kConfigId, kConfigName, serializeConfig(cfg));
    }

    // 2. 确保日志目录存在
    QDir().mkpath(logDir);

    // 3. 创建双 sink
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        (logDir + QStringLiteral("/app.log")).toStdString(),
        cfg.maxFileSize,
        cfg.fileCount);
    fileSink->set_level(toSpdLevel(cfg.fileLevel));

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(toSpdLevel(cfg.consoleLevel));

    // 4. 组装 logger
    auto logger = std::make_shared<spdlog::logger>(
        "main",
        spdlog::sinks_init_list{fileSink, consoleSink});
    logger->set_level(spdlog::level::trace);      // 不限流，由各 sink 独立过滤
    logger->flush_on(spdlog::level::debug);
    spdlog::set_default_logger(logger);

    // 5. 设置日志格式
    //    %Y-%m-%d %H:%M:%S.%e = 时间戳（毫秒）
    //    %^%l%$               = 彩色级别
    //    %n                   = logger 名称（用于模块归属）
    //    %t                   = 线程 ID
    //    %s:%#                = 源文件:行号
    //    %!                   = 函数名
    //    %v                   = 实际日志消息
    spdlog::set_pattern(
        QStringLiteral("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [thread %t] [%s:%#] [%!] %v")
            .toStdString());

    // 6. 为各模块创建命名 logger，共享同一对 sink
    auto sinks = logger->sinks();
    auto makeModuleLogger = [&sinks](const std::string& name) {
        auto l = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        l->set_level(spdlog::level::trace);      // 不限流，由各 sink 独立过滤
        l->flush_on(spdlog::level::debug);
        spdlog::register_logger(l);
    };
    makeModuleLogger("AppCoordinator");
    makeModuleLogger("DataSync");
    makeModuleLogger("AnkiConnect");
    makeModuleLogger("Hold");
    makeModuleLogger("SceneOrchestrator");
    makeModuleLogger("QianWenAI");
    makeModuleLogger("WenXinAI");
}

} // namespace LogManage

#endif // LOGMANAGE_H
