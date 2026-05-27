#include "HoldWorker.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

HoldWorker::HoldWorker(const QString &dataDir, QObject *parent)
    : QObject(parent)
{
    QDir().mkpath(dataDir);
}

void HoldWorker::doWrite(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit writeError(path, file.errorString());
        return;
    }

    if (file.write(data) != data.size()) {
        emit writeError(path, file.errorString());
        return;
    }

    if (!file.commit()) {
        emit writeError(path, file.errorString());
        return;
    }

    emit writeCompleted(path);
}

void HoldWorker::doRemove(const QString &path)
{
    QFile::remove(path);
}
