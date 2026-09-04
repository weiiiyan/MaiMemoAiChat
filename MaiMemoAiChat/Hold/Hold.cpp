#include "Hold.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <spdlog/spdlog.h>

namespace {
spdlog::logger* logger() {
    static auto instance = spdlog::get("Hold");
    return instance.get();
}
} // namespace

Hold::Hold(const QString &dataDir, QObject *parent)
    : QObject(parent)
    , m_dataDir(dataDir)
{
    QDir().mkpath(m_dataDir);
}

QByteArray Hold::load(const QStringList &id, const QString &name) const
{
    QString path = filePath(id, name);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        SPDLOG_LOGGER_TRACE(logger(), "load not found: {}", path.toStdString());
        return {};
    }
    QByteArray data = file.readAll();
    SPDLOG_LOGGER_TRACE(logger(), "load {} ({} bytes)", path.toStdString(), data.size());
    return data;
}

bool Hold::save(const QStringList &id, const QString &name, const QByteArray &data)
{
    QString path = filePath(id, name);
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        SPDLOG_LOGGER_ERROR(logger(), "save failed to open: {}", path.toStdString());
        return false;
    }
    file.write(data);
    if (!file.commit()) {
        SPDLOG_LOGGER_ERROR(logger(), "save failed to commit: {}", path.toStdString());
        return false;
    }
    SPDLOG_LOGGER_DEBUG(logger(), "save {} ({} bytes)", path.toStdString(), data.size());
    return true;
}

bool Hold::remove(const QStringList &id, const QString &name)
{
    QString path = filePath(id, name);
    bool ok = QFile::remove(path);
    SPDLOG_LOGGER_DEBUG(logger(), "remove {} -> {}", path.toStdString(), ok ? "ok" : "failed");
    return ok;
}

bool Hold::exists(const QStringList &id, const QString &name) const
{
    return QFile::exists(filePath(id, name));
}

QString Hold::filePath(const QStringList &id, const QString &name) const
{
    return m_dataDir + QLatin1Char('/') + id.join(QLatin1Char('/')) + QLatin1Char('/') + name;
}
