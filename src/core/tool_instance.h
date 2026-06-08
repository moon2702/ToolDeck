#ifndef TOOL_INSTANCE_H
#define TOOL_INSTANCE_H

#include <QObject>
#include <QProcess>
#include <QElapsedTimer>
#include "tool_manifest.h"

/// Running state of a tool instance
enum class ToolState {
    Idle,
    Starting,
    Running,
    Finished,
    Failed,
    Stopped
};

/// Wraps a QProcess for a single tool execution.
/// Tracks state, buffers output, emits signals for UI updates.
class ToolInstance : public QObject
{
    Q_OBJECT

public:
    explicit ToolInstance(const ToolManifest &manifest, QObject *parent = nullptr);
    ~ToolInstance() override;

    /// Start executing the tool
    /// @param extraArgs  additional arguments appended after manifest.args
    void start(const QStringList &extraArgs = {});

    /// Stop the running tool (SIGTERM, then SIGKILL after timeout)
    void stop();

    /// Restart the tool
    void restart();

    // -- Accessors --

    const ToolManifest &manifest() const { return manifest_; }
    ToolState state() const { return state_; }
    QProcess *process() const { return process_; }
    qint64 elapsedMs() const { return timer_.elapsed(); }
    int exitCode() const { return exitCode_; }
    QString outputBuffer() const { return outputBuffer_; }
    QString errorBuffer() const { return errorBuffer_; }

    /// Human-readable state string
    QString stateString() const;

signals:
    /// Emitted when state changes
    void stateChanged(ToolState newState);

    /// Emitted when new stdout data arrives
    void outputReady(const QString &text);

    /// Emitted when new stderr data arrives
    void errorReady(const QString &text);

    /// Emitted when the tool finishes (exit code available)
    void finished(int exitCode);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void setState(ToolState newState);
    QString resolveCommand() const;

    ToolManifest manifest_;
    QProcess *process_;
    ToolState state_ = ToolState::Idle;
    int exitCode_ = -1;
    QString outputBuffer_;
    QString errorBuffer_;
    QElapsedTimer timer_;
};

#endif // TOOL_INSTANCE_H
