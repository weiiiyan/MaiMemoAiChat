#include "mainwindow.h"

#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
}

MainWindow::~MainWindow() = default;

// ── IUIModule 接口实现 ──

void MainWindow::showChatView(const QString &sessionId)
{
    m_currentSessionId = sessionId;
    m_chatView->clear();

    QString header = QStringLiteral("=== Session: %1 ===\n").arg(sessionId);
    m_chatView->setPlainText(header);
}

void MainWindow::showMessage(const QString &sessionId, const QString &message)
{
    Q_UNUSED(sessionId);
    // 追加文本——流式 chunk 直接拼接，无需换行
    QTextCursor cursor = m_chatView->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(message);
    m_chatView->ensureCursorVisible();
}

void MainWindow::showSummary(const QString &sessionId)
{
    QString summary = QStringLiteral("\n--- Session %1 ended ---\n").arg(sessionId);
    QTextCursor cursor = m_chatView->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(summary);
    m_chatView->ensureCursorVisible();
}

void MainWindow::setDueEntryCount(int count)
{
    m_dueLabel->setText(QStringLiteral("Due: %1").arg(count));
}

void MainWindow::showStatus(const QString &status)
{
    m_statusLabel->setText(status);

    // 连接就绪后启用操作按钮
    bool ready = status.startsWith(QStringLiteral("Ready"));
    m_reviewBtn->setEnabled(ready || status == QStringLiteral("In session"));
    m_sendBtn->setEnabled(true); // 始终可输入，发送时由 AppCoordinator 判断
}

void MainWindow::appendSystemMessage(const QString &message)
{
    QTextCursor cursor = m_chatView->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (!m_chatView->document()->isEmpty())
        cursor.insertText(QStringLiteral("\n"));
    cursor.insertText(QStringLiteral("[System] %1").arg(message));
    m_chatView->ensureCursorVisible();
}

// ── 私有 slot ──

void MainWindow::onSendClicked()
{
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty())
        return;

    // 在聊天区展示用户消息
    QTextCursor cursor = m_chatView->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral("\n[You] %1\n").arg(text));
    m_chatView->ensureCursorVisible();

    m_inputEdit->clear();

    emit messageSent(m_currentSessionId, text);
}

// ── 私有方法 ──

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("MaiMemo AI Chat"));
    resize(640, 480);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ── 顶部工具栏 ──
    auto *toolbar = new QHBoxLayout;

    m_statusLabel = new QLabel(QStringLiteral("Offline"), this);

    m_dueLabel = new QLabel(QStringLiteral("Due: 0"), this);

    m_reviewBtn = new QPushButton(QStringLiteral("Start Review"), this);
    m_reviewBtn->setEnabled(false);
    connect(m_reviewBtn, &QPushButton::clicked, this, &MainWindow::reviewRequested);

    toolbar->addWidget(m_statusLabel);
    toolbar->addWidget(m_dueLabel);
    toolbar->addStretch();
    toolbar->addWidget(m_reviewBtn);

    mainLayout->addLayout(toolbar);

    // ── 聊天显示区 ──
    m_chatView = new QTextEdit(this);
    m_chatView->setReadOnly(true);
    mainLayout->addWidget(m_chatView, 1);

    // ── 底部输入栏 ──
    auto *inputBar = new QHBoxLayout;

    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setPlaceholderText(QStringLiteral("Type your message..."));
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);

    m_sendBtn = new QPushButton(QStringLiteral("Send"), this);
    m_sendBtn->setEnabled(false);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    inputBar->addWidget(m_inputEdit, 1);
    inputBar->addWidget(m_sendBtn);

    mainLayout->addLayout(inputBar);
}
