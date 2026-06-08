#include "tool_instance.h"

#include <QDir>
#include <QDebug>

ToolInstance::ToolInstance(const ToolManifest &manifest, QObject *parent)
    : QObject(parent)
    , manifest_(manifest)
    , process_(new QProcess(this))
{
    connect(process_, &QProcess::readyReadStandardOutput,
            this, &ToolInstance::onReadyReadStdout);
    connect(process_, &QProcess::readyReadStandardError,
            this, &ToolInstance::onReadyReadStderr);
    connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ToolInstance::onProcessFinished);
    connect(process_, &QProcess::errorOccurred,
            this, &ToolInstance::onProcessError);

    // Merge stdout and stderr for simpler output handling
    process_->setProcessChannelMode(QProcess::MergedChannels);
}

ToolInstance::~ToolInstance()
{
    if (process_->state() != QProcess::NotRunning) {
        process_->terminate();
        if (!process_->waitForFinished(3000)) {
            process_->kill();
        }
    }
}

QString ToolInstance::resolveCommand() const
{
    QString cmd = manifest_.command;

    // If the command is a relative path, resolve relative to the manifest directory
    if (QDir::isRelativePath(cmd)) {
        QString resolved = manifest_.directoryPath + "/" + cmd;
        if (QFile::exists(resolved)) {
            return resolved;
        }
    }

    return cmd;
}

void ToolInstance::start(const QStringList &extraArgs)
{
    if (state_ == ToolState::Running || state_ == ToolState::Starting) {
        qDebug() << "Tool" << manifest_.name << "is already running";
        return;
    }

    outputBuffer_.clear();
    errorBuffer_.clear();
    exitCode_ = -1;

    QString program = resolveCommand();
    QStringList args = manifest_.args;
    args.append(extraArgs);

    // Set working directory
    QString workDir = manifest_.workingDir;
    if (QDir::isRelativePath(workDir)) {
        workDir = manifest_.directoryPath + "/" + workDir;
    }
    process_->setWorkingDirectory(workDir);

    setState(ToolState::Starting);
    timer_.start();

    qDebug() << "Starting tool:" << program << args;
    process_->start(program, args);

    if (process_->waitForStarted(5000)) {
        setState(ToolState::Running);
    } else {
        qWarning() << "Failed to start tool:" << manifest_.name;
        setState(ToolState::Failed);
    }
}

void ToolInstance::stop()
{
    if (state_ != ToolState::Running && state_ != ToolState::Starting) {
        return;
    }

    qDebug() << "Stopping tool:" << manifest_.name;
    process_->terminate();

    if (!process_->waitForFinished(3000)) {
        qWarning() << "Tool did not terminate, killing:" << manifest_.name;
        process_->kill();
        process_->waitForFinished(2000);
    }

    setState(ToolState::Stopped);
}

void ToolInstance::restart()
{
    stop();
    start();
}

QString ToolInstance::stateString() const
{
    switch (state_) {
    case ToolState::Idle:     return "空闲";
    case ToolState::Starting: return "启动中...";
    case ToolState::Running:  return "运行中";
    case ToolState::Finished: return QString("已完成 (%1)").arg(exitCode_);
    case ToolState::Failed:   return "失败";
    case ToolState::Stopped:  return "已停止";
    }
    return "未知";
}

void ToolInstance::setState(ToolState newState)
{
    if (state_ != newState) {
        state_ = newState;
        emit stateChanged(newState);
    }
}

void ToolInstance::onReadyReadStdout()
{
    QByteArray data = process_->readAllStandardOutput();
    outputBuffer_ += QString::fromUtf8(data);
    emit outputReady(QString::fromUtf8(data));
}

void ToolInstance::onReadyReadStderr()
{
    QByteArray data = process_->readAllStandardError();
    errorBuffer_ += QString::fromUtf8(data);
    emit errorReady(QString::fromUtf8(data));
}

void ToolInstance::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    exitCode_ = exitCode;
    Q_UNUSED(exitStatus);
    qDebug() << "Tool" << manifest_.name << "finished with exit code:" << exitCode;

    if (state_ != ToolState::Stopped) {
        setState(ToolState::Finished);
    }

    emit finished(exitCode);
}

void ToolInstance::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    qWarning() << "Tool" << manifest_.name << "error:" << process_->errorString();

    if (state_ == ToolState::Starting || state_ == ToolState::Running) {
        setState(ToolState::Failed);
    }
}
