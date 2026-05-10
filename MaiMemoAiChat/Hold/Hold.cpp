#include "Hold.h"
#include "HoldWorker.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QTimer>

Hold::Hold(const QString &dataDir, QObject *parent)
    : QObject(parent)
    , m_dataDir(dataDir)
{
    // 确保数据根目录存在
    QDir().mkpath(m_dataDir);

    // 创建 worker 线程，HoldWorker 在其中执行所有文件 I/O
    m_workerThread = new QThread(this);
    m_worker = new HoldWorker(dataDir);
    m_worker->moveToThread(m_workerThread);

    // 线程退出时自动清理 worker
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // worker 写入完成 → 将磁盘路径还原为 (id, name) 再转发信号给上层调用方
    connect(m_worker, &HoldWorker::writeCompleted, this, [this](const QString &path) {
        QStringList parts = QDir(m_dataDir).relativeFilePath(path).split(QLatin1Char('/'));
        if (parts.size() < 2)
            return;
        QString name = parts.takeLast(); // 最后一段是 name
        emit saveCompleted(parts, name); // 剩下的是 id
    });
    connect(m_worker, &HoldWorker::writeError, this, [this](const QString &path, const QString &error) {
        QStringList parts = QDir(m_dataDir).relativeFilePath(path).split(QLatin1Char('/'));
        if (parts.size() < 2)
            return;
        QString name = parts.takeLast();
        emit saveError(parts, name, error);
    });

    m_workerThread->start();
}

Hold::~Hold()
{
    // 停止所有防抖定时器，丢弃尚未刷盘的写入
    for (auto it = m_timers.begin(); it != m_timers.end(); ++it) {
        it.value()->stop();
        delete it.value();
    }
    m_timers.clear();
    m_pendingWrites.clear();

    m_workerThread->quit();
    m_workerThread->wait();
}

QByteArray Hold::load(const QStringList &id, const QString &name) const
{
    QString key = makeKey(id, name);

    // 优先从防抖缓存读取，保证读到最新未刷盘数据
    {
        QMutexLocker lock(&m_mutex);
        if (m_pendingWrites.contains(key))
            return m_pendingWrites.value(key);
    }

    // 缓存未命中，直接读磁盘
    QFile file(filePath(id, name));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

void Hold::save(const QStringList &id, const QString &name, const QByteArray &data)
{
    QString key = makeKey(id, name);

    {
        QMutexLocker lock(&m_mutex);
        m_pendingWrites[key] = data;

        // 如果已有定时器在跑，重置它（实现"5 秒内再次写入则重新计时"的防抖逻辑）
        if (m_timers.contains(key)) {
            m_timers[key]->stop();
            delete m_timers.take(key);
        }

        // 创建一次性定时器：5 秒后触发 flushPendingWrite 将数据派发至 worker 线程
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        m_timers[key] = timer;
        connect(timer, &QTimer::timeout, this, [this, key]() {
            flushPendingWrite(key);
        });
        timer->start(5000);
    }
}

bool Hold::remove(const QStringList &id, const QString &name)
{
    QString key = makeKey(id, name);

    // 取消该 key 的防抖定时器和待写入数据
    {
        QMutexLocker lock(&m_mutex);
        if (m_timers.contains(key)) {
            m_timers[key]->stop();
            delete m_timers.take(key);
        }
        m_pendingWrites.remove(key);
    }

    // 删除请求通过跨线程信号槽派发至 worker 线程执行
    QMetaObject::invokeMethod(m_worker, "doRemove",
                              Qt::QueuedConnection,
                              Q_ARG(QString, filePath(id, name)));
    return true;
}

bool Hold::exists(const QStringList &id, const QString &name) const
{
    QString key = makeKey(id, name);

    // 先查防抖缓存（可能有一条尚未刷盘的 save）
    {
        QMutexLocker lock(&m_mutex);
        if (m_pendingWrites.contains(key))
            return true;
    }

    return QFile::exists(filePath(id, name));
}

QString Hold::makeKey(const QStringList &id, const QString &name)
{
    // key 格式: "ns1/ns2/.../name"，与磁盘目录结构一致
    return id.join(QLatin1Char('/')) + QLatin1Char('/') + name;
}

QStringList Hold::parseKey(const QString &key)
{
    return key.split(QLatin1Char('/'));
}

QString Hold::filePath(const QStringList &id, const QString &name) const
{
    return m_dataDir + QLatin1Char('/') + id.join(QLatin1Char('/')) + QLatin1Char('/') + name;
}

void Hold::flushPendingWrite(const QString &key)
{
    QByteArray data;
    {
        QMutexLocker lock(&m_mutex);
        // 取出数据并从两个容器中移除
        data = m_pendingWrites.take(key);
        delete m_timers.take(key);
    }

    if (data.isEmpty())
        return;

    // 从 key 还原 id 和 name
    QStringList parts = key.split(QLatin1Char('/'));
    if (parts.size() < 2)
        return;
    QString name = parts.takeLast();
    QStringList id = parts;

    // 跨线程派发写入任务
    QMetaObject::invokeMethod(m_worker, "doWrite",
                              Qt::QueuedConnection,
                              Q_ARG(QString, filePath(id, name)),
                              Q_ARG(QByteArray, data));
}
