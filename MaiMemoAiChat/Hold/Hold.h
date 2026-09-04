#pragma once

#include <QObject>
#include <QByteArray>
#include <QStringList>

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
 * 写入策略：同步写入，使用 QSaveFile 保证原子性（先写临时文件再 rename）。
 */
class Hold : public QObject
{
    Q_OBJECT

public:
    explicit Hold(const QString &dataDir, QObject *parent = nullptr);

    /// 读取指定 key 的数据，若不存在返回空 QByteArray
    QByteArray load(const QStringList &id, const QString &name) const;

    /// 同步写入数据（原子写入），成功返回 true
    bool save(const QStringList &id, const QString &name, const QByteArray &data);

    /// 删除指定 key 的数据
    bool remove(const QStringList &id, const QString &name);

    /// 检查 key 是否存在
    bool exists(const QStringList &id, const QString &name) const;

private:
    QString filePath(const QStringList &id, const QString &name) const;

    QString m_dataDir;
};
