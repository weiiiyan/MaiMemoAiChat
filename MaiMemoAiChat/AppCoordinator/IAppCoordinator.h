#pragma once

#include <QObject>
#include "../DataSync/MemEntry.h"
#include "../SceneOrchestrator/SceneTypes.h"

class IUIModule;

/// 应用配置 — initialize() 时传入
struct AppConfig {
    QString dataDir;                       ///< Hold 数据根目录
    SRSConfig srsConfig;                   ///< SRS 引擎连接配置
    SceneConfig sceneConfig;               ///< AI 场景配置
    SceneType defaultSceneType = SceneType::Reading;
    int dueEntryLimit = 20;                ///< 每次复习取多少条到期条目
};

/**
 * @brief 应用协调模块主接口 — 整个应用的入口与中枢
 *
 * 职责：
 * - 初始化/销毁所有子模块（DataSync、SceneOrchestrator、Hold）
 * - 监听 IUIModule 信号，编排完整业务流程
 * - 协调 DataSync ↔ SceneOrchestrator 之间的数据流
 *
 * AppCoordinator 不发射信号，它直接调用 IUIModule 方法驱动 UI。
 * UI 通过 IUIModule 信号触发业务流程，不直接调用 AppCoordinator 业务方法。
 */
class IAppCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit IAppCoordinator(QObject *parent = nullptr) : QObject(parent) {}
    ~IAppCoordinator() override = default;

    /// 初始化应用：加载配置 → 初始化子模块 → 连接信号 → 恢复上次会话
    /// @param uiObject QObject 指针（MainWindow），用于 connect() 信号连接
    /// @param ui       IUIModule 接口指针，用于调用 UI 方法（与 uiObject 指向同一对象）
    virtual bool initialize(QObject *uiObject, IUIModule *ui, const AppConfig &config) = 0;

    /// 安全关闭：保存状态 → 断开引擎 → 释放资源
    virtual void shutdown() = 0;

    /// 是否已完成初始化
    virtual bool isInitialized() const = 0;

    /// 触发复习流程：getDueEntries → createSession → showChatView
    virtual void startReview() = 0;

    /// 获取到期条目数，供 UI 角标/提示展示
    virtual int dueEntryCount() const = 0;
};
