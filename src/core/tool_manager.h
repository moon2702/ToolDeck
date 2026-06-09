#ifndef TOOL_MANAGER_H
#define TOOL_MANAGER_H

#include <QObject>
#include <QHash>
#include "tool_instance.h"

/// Manages lifecycle of running tool instances.
/// Start / stop / restart tools, limit concurrency, track active instances.
class ToolManager : public QObject
{
    Q_OBJECT

public:
    explicit ToolManager(QObject *parent = nullptr);
    ~ToolManager() override;

    /// Start a tool by name. Returns the instance, or nullptr on failure.
    /// @param extraArgs   additional arguments appended after manifest.args
    /// @param instanceKey override storage key (default: manifest.name).
    ///                    Use a distinct key to allow multiple concurrent instances of the same tool.
    ToolInstance *startTool(const ToolManifest &manifest, const QStringList &extraArgs = {},
                            const QString &instanceKey = {});

    /// Stop a running tool by name
    void stopTool(const QString &name);

    /// Restart a tool
    void restartTool(const QString &name);

    /// Stop all running tools
    void stopAll();

    /// Get a running instance by name, or nullptr
    ToolInstance *instance(const QString &name) const;

    /// All currently tracked instances
    const QHash<QString, ToolInstance *> &instances() const { return instances_; }

    /// Whether any tool is currently running
    bool hasRunning() const;

    /// Maximum concurrent tools (0 = unlimited)
    void setMaxConcurrency(int max) { maxConcurrency_ = max; }
    int maxConcurrency() const { return maxConcurrency_; }

signals:
    /// Emitted when a tool instance is created
    void instanceCreated(const QString &name, ToolInstance *instance);

    /// Emitted when an instance is removed
    void instanceRemoved(const QString &name);

private:
    QHash<QString, ToolInstance *> instances_;
    int maxConcurrency_ = 0; // 0 = unlimited
};

#endif // TOOL_MANAGER_H
