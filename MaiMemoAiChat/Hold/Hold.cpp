#include "Hold.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

Hold::Hold(const QString &dataDir, QObject *parent)
    : QObject(parent)
    , m_dataDir(dataDir)
{
    QDir().mkpath(m_dataDir);
}

QByteArray Hold::load(const QStringList &id, const QString &name) const
{
    QFile file(filePath(id, name));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

void Hold::save(const QStringList &id, const QString &name, const QByteArray &data)
{
    QString path = filePath(id, name);
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(data);
    file.commit();
}

bool Hold::remove(const QStringList &id, const QString &name)
{
    return QFile::remove(filePath(id, name));
}

bool Hold::exists(const QStringList &id, const QString &name) const
{
    return QFile::exists(filePath(id, name));
}

QString Hold::filePath(const QStringList &id, const QString &name) const
{
    return m_dataDir + QLatin1Char('/') + id.join(QLatin1Char('/')) + QLatin1Char('/') + name;
}
