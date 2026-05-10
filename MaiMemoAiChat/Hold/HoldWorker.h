#pragma once

#include <QObject>
#include <QByteArray>

/**
 * @brief Hold 的文件 I/O 工作对象，驻留在 worker 线程。
 *
 * 所有磁盘操作在此对象中执行，通过 Qt 跨线程信号槽与主线程 Hold 通信。
 * 使用 QTemporaryFile + rename 策略保证写入原子性。
 */
class HoldWorker : public QObject
{
    Q_OBJECT

public:
    explicit HoldWorker(const QString &dataDir, QObject *parent = nullptr);

public slots:
    /**
     * @brief 原子写入文件：先写临时文件，成功后再 rename 到目标路径
     * @param path 目标文件绝对路径
     * @param data 二进制数据
     */
    void doWrite(const QString &path, const QByteArray &data);

    /**
     * @brief 同步删除文件
     */
    void doRemove(const QString &path);

signals:
    /// 写入成功
    void writeCompleted(const QString &path);
    /// 写入失败，携带错误描述
    void writeError(const QString &path, const QString &error);
};
