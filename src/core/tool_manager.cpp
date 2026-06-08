#include "tool_manager.h"
#include <QDebug>

ToolManager::ToolManager(QObject *parent)
    : QObject(parent)
{
}

ToolManager::~ToolManager()
{
    stopAll();
}

ToolInstance *ToolManager::startTool(const ToolManifest &manifest, const QStringList &extraArgs)
{
    // Check if already running
    if (instances_.contains(manifest.name)) {
        auto *existing = instances_[manifest.name];
        if (existing->state() == ToolState::Running ||
            existing->state() == ToolState::Starting) {
            qDebug() << "Tool" << manifest.name << "is already running";
            return existing;
        }
        // Clean up finished/stopped instance
        existing->deleteLater();
        instances_.remove(manifest.name);
    }

    // Check concurrency limit
    if (maxConcurrency_ > 0) {
        int running = 0;
        for (const auto *inst : instances_) {
            if (inst->state() == ToolState::Running || inst->state() == ToolState::Starting) {
                running++;
            }
        }
        if (running >= maxConcurrency_) {
            qWarning() << "Max concurrency reached (" << maxConcurrency_ << "), cannot start" << manifest.name;
            return nullptr;
        }
    }

    auto *instance = new ToolInstance(manifest, this);

    // Clean up when finished (for oneshot tools)
    connect(instance, &ToolInstance::finished, this, [this, name = manifest.name](int exitCode) {
        Q_UNUSED(exitCode);
        qDebug() << "Tool" << name << "completed, cleaning up instance";
    });

    instances_[manifest.name] = instance;
    instance->start(extraArgs);

    emit instanceCreated(manifest.name, instance);
    return instance;
}

void ToolManager::stopTool(const QString &name)
{
    auto it = instances_.find(name);
    if (it != instances_.end()) {
        (*it)->stop();
    }
}

void ToolManager::restartTool(const QString &name)
{
    auto it = instances_.find(name);
    if (it != instances_.end()) {
        (*it)->restart();
    }
}

void ToolManager::stopAll()
{
    for (auto *inst : instances_) {
        inst->stop();
    }
    qDeleteAll(instances_);
    instances_.clear();
}

ToolInstance *ToolManager::instance(const QString &name) const
{
    return instances_.value(name, nullptr);
}

bool ToolManager::hasRunning() const
{
    for (const auto *inst : instances_) {
        if (inst->state() == ToolState::Running || inst->state() == ToolState::Starting) {
            return true;
        }
    }
    return false;
}
