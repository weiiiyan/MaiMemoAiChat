#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "IUIModule.h"
#include "MemEntry.h"

class QTextEdit;
class QLineEdit;
class QPushButton;
class QLabel;

class MainWindow : public QMainWindow, public IUIModule
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // IUIModule 接口
    void showChatView(const QString &sessionId) override;
    void showMessage(const QString &sessionId, const QString &message) override;
    void showSummary(const QString &sessionId) override;
    void setDueEntryCount(int count) override;
    void setInitialized(bool initialized) override;

signals:
    void messageSent(const QString &sessionId, const QString &content);
    void reviewRequested();
    void reviewAnswered(const QString &sessionId, qint64 cardId, int ease);
    void settingsChanged(const SRSConfig &config);

private slots:
    void onSendClicked();

private:
    void setupUi();

    QString m_currentSessionId;

    QLabel *m_statusLabel;
    QLabel *m_dueLabel;
    QPushButton *m_reviewBtn;
    QTextEdit *m_chatView;
    QLineEdit *m_inputEdit;
    QPushButton *m_sendBtn;
};

#endif // MAINWINDOW_H
