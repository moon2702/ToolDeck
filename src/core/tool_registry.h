#ifndef TOOL_REGISTRY_H
#define TOOL_REGISTRY_H

#include <QObject>
#include <QVector>
#include <QFileSystemWatcher>
#include "tool_manifest.h"

/// Discovers, loads, and indexes all available tool manifests.
/// Scans standard tool directories and watches for changes.
class ToolRegistry : public QObject
{
    Q_OBJECT

public:
    explicit ToolRegistry(QObject *parent = nullptr);
    ~ToolRegistry() override = default;

    /// Scan all tool directories and load manifests
    void reload();

    /// Add a custom search path for tools
    void addSearchPath(const QString &path);

    /// Get all loaded tool manifests
    const QVector<ToolManifest> &tools() const { return tools_; }

    /// Find a tool by name
    const ToolManifest *findByName(const QString &name) const;

    /// Get tools grouped by category
    QMap<QString, QVector<ToolManifest>> toolsByCategory() const;

    /// Get unique categories, sorted
    QStringList categories() const;

    /// Number of loaded tools
    int count() const { return tools_.size(); }

signals:
    /// Emitted after tools are reloaded
    void toolsChanged();

    /// Emitted when a specific tool is added or updated
    void toolUpdated(const QString &name);

private:
    /// Scan a single directory for tool manifests
    void scanDirectory(const QString &dirPath);

    /// Resolve standard search paths
    QStringList defaultSearchPaths() const;

    QVector<ToolManifest> tools_;
    QFileSystemWatcher *watcher_;
    QStringList searchPaths_;
};

#endif // TOOL_REGISTRY_H
