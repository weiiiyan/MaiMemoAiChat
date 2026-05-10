#include "HoldWorker.h"

#include <QDir>
#include <QFile>
#include <QTemporaryFile>

HoldWorker::HoldWorker(const QString &dataDir, QObject *parent)
    : QObject(parent)
{
    QDir().mkpath(dataDir);
}

void HoldWorker::doWrite(const QString &path, const QByteArray &data)
{
    // 先确保目标目录存在
    QDir().mkpath(QFileInfo(path).absolutePath());

    // 在同目录下创建临时文件（以 .tmp_ 为前缀）
    QTemporaryFile tempFile(QFileInfo(path).absolutePath() + QStringLiteral("/.tmp_XXXXXX"));
    tempFile.setFileTemplate(tempFile.fileTemplate());

    if (!tempFile.open()) {
        emit writeError(path, QStringLiteral("Failed to create temp file: ") + tempFile.errorString());
        return;
    }

    // 将全部数据写入临时文件
    qint64 written = tempFile.write(data);
    if (written != data.size()) {
        emit writeError(path, QStringLiteral("Incomplete write: ") + tempFile.errorString());
        return;
    }
    tempFile.flush();

    // 原子 rename：临时文件 → 目标路径
    // 如果 rename 失败（跨卷场景），回退到 copy + remove
    if (!tempFile.rename(path)) {
        QFile::remove(path);
        if (!tempFile.copy(path)) {
            emit writeError(path, QStringLiteral("Failed to commit file: ") + tempFile.errorString());
            return;
        }
        tempFile.remove();
    }

    emit writeCompleted(path);
}

void HoldWorker::doRemove(const QString &path)
{
    QFile::remove(path);
}
