#pragma once

#include <QObject>
#include <QByteArray>
#include <QStringList>
#include <QHash>
#include <QMutex>
#include <memory>

class QThread;
class QTimer;
class HoldWorker;

/**
 * @brief 持久化模块 — 应用内所有数据的统一存储入口。
 *
 * 存储模型：以 (id, name) 二元组定位一条数据。
 * - id:  嵌套的命名空间，如 @c ["sessions", "session-abc"]
 * - name: 命名空间下的条目名，如 @c "metadata"
 *
 * 磁盘布局：@c {dataDir}/{id用'/'连接}/{name}
 * 例如 @c /data/sessions/session-abc/metadata
 *
 * 写入策略：
 * - 异步：实际文件 I/O 在 worker 线程执行，不阻塞主线程。
 * - 防抖：同一 key 在 5 秒内多次 save()，仅最后一次触发写入。
 * - 原子：先写临时文件再 rename，防止磁盘满或写入中断损坏数据。
 *
 * 读取策略：
 * - 优先查防抖缓存（尚未刷盘的 pending write），未命中则直接读磁盘文件。
 */
class Hold : public QObject
{
    Q_OBJECT

public:
    /**
     * @param dataDir 数据根目录，会在构造时自动创建
     */
    explicit Hold(const QString &dataDir, QObject *parent = nullptr);
    ~Hold() override;

    /**
     * @brief 读取指定 key 的数据
     * @return 若 key 不存在返回空 QByteArray
     */
    QByteArray load(const QStringList &id, const QString &name) const;

    /**
     * @brief 异步写入数据（5 秒防抖）
     *
     * 写入成功后发出 saveCompleted()，失败发出 saveError()。
     */
    void save(const QStringList &id, const QString &name, const QByteArray &data);

    /**
     * @brief 删除指定 key 的数据，同时取消该 key 的待写入任务
     * @return 始终返回 true（删除请求已派发至 worker 线程）
     */
    bool remove(const QStringList &id, const QString &name);

    /**
     * @brief 检查 key 是否存在（含防抖缓存中尚未刷盘的数据）
     */
    bool exists(const QStringList &id, const QString &name) const;

signals:
    /// 某条数据已成功写入磁盘
    void saveCompleted(const QStringList &id, const QString &name);
    /// 写入失败，携带错误描述
    void saveError(const QStringList &id, const QString &name, const QString &error);

private:
    /// 将 (id, name) 合并为内部索引 key，格式: "ns1/ns2/.../name"
    static QString makeKey(const QStringList &id, const QString &name);
    /// 从内部索引 key 拆分回各段
    static QStringList parseKey(const QString &key);
    /// 计算磁盘文件路径
    QString filePath(const QStringList &id, const QString &name) const;

    /**
     * @brief 防抖定时器到期回调 — 将缓存数据派发至 worker 线程执行实际写入
     */
    void flushPendingWrite(const QString &key);

    QString m_dataDir;              ///< 数据根目录
    QThread *m_workerThread;        ///< 文件 I/O 工作线程
    HoldWorker *m_worker;           ///< 驻留在 worker 线程的工作对象

    mutable QMutex m_mutex;                    ///< 保护以下两个容器的互斥锁
    QHash<QString, QTimer *> m_timers;          ///< 每个 key 的 5 秒防抖定时器
    QHash<QString, QByteArray> m_pendingWrites; ///< 防抖缓存，等待刷盘的数据
};
