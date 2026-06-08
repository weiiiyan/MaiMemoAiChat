#include "Hold.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <spdlog/spdlog.h>

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
        SPDLOG_TRACE("Hold::load not found: {}", path.toStdString());
        return {};
    }
    QByteArray data = file.readAll();
    SPDLOG_TRACE("Hold::load {} ({} bytes)", path.toStdString(), data.size());
    return data;
}

void Hold::save(const QStringList &id, const QString &name, const QByteArray &data)
{
    QString path = filePath(id, name);
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        SPDLOG_ERROR("Hold::save failed to open: {}", path.toStdString());
        return;
    }
    file.write(data);
    if (!file.commit()) {
        SPDLOG_ERROR("Hold::save failed to commit: {}", path.toStdString());
    }
    SPDLOG_DEBUG("Hold::save {} ({} bytes)", path.toStdString(), data.size());
}

bool Hold::remove(const QStringList &id, const QString &name)
{
    QString path = filePath(id, name);
    bool ok = QFile::remove(path);
    SPDLOG_DEBUG("Hold::remove {} -> {}", path.toStdString(), ok ? "ok" : "failed");
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
