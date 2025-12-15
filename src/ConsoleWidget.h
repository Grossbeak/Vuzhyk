#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QProcess>
#include <QVBoxLayout>
#include <QPointer>

class ConsoleWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConsoleWidget(QWidget *parent = nullptr);
    ~ConsoleWidget();

    void setPythonPath(const QString &pythonPath);
    void setWorkingDirectory(const QString &dir);
    void clear();
    void write(const QString &text);
    void writeCommand(const QString &command);
    bool isRunning() const;
    void terminate();
    void ensureStarted();

signals:
    void outputReceived(const QString &text, bool isError);
    void commandFinished(int exitCode);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onInputReturnPressed();

private:
    void startConsole();
    void appendOutput(const QString &text, bool isError = false);
    void appendInput(const QString &text);
    QString getPythonDirectory() const;

    QPlainTextEdit *m_output;
    QLineEdit *m_input;
    QPointer<QProcess> m_process;
    QString m_pythonPath;
    QString m_currentDirectory;
    bool m_isRunningCommand;
};

