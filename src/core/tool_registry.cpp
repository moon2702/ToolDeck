#include "tool_registry.h"

#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QSet>
#include <QDebug>

ToolRegistry::ToolRegistry(QObject *parent)
    : QObject(parent)
    , watcher_(new QFileSystemWatcher(this))
{
    connect(watcher_, &QFileSystemWatcher::directoryChanged,
            this, &ToolRegistry::reload);
}

QStringList ToolRegistry::defaultSearchPaths() const
{
    QStringList paths;

    // XDG config path
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    paths << configPath + "/tooldeck/tools";

    // Project-local examples directory (for development)
    paths << QCoreApplication::applicationDirPath() + "/../examples";
    paths << QCoreApplication::applicationDirPath() + "/../../examples";

    return paths;
}

void ToolRegistry::addSearchPath(const QString &path)
{
    if (!searchPaths_.contains(path)) {
        searchPaths_.append(path);
    }
}

void ToolRegistry::reload()
{
    tools_.clear();

    // Build the full search path list
    QStringList allPaths = defaultSearchPaths();
    for (const auto &p : searchPaths_) {
        if (!allPaths.contains(p)) {
            allPaths.append(p);
        }
    }

    for (const auto &path : allPaths) {
        scanDirectory(path);
    }

    // Deduplicate by tool name — first found wins (config path scanned first)
    QVector<ToolManifest> deduped;
    QSet<QString> seen;
    for (const auto &tool : tools_) {
        if (!seen.contains(tool.name)) {
            seen.insert(tool.name);
            deduped.append(tool);
        }
    }
    tools_ = deduped;

    qDebug() << "ToolRegistry: loaded" << tools_.size() << "tools from" << allPaths;
    emit toolsChanged();
}

void ToolRegistry::scanDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return;
    }

    // Watch this directory for changes
    if (!watcher_->directories().contains(dirPath)) {
        watcher_->addPath(dirPath);
    }

    // Look for manifest.json in each subdirectory
    QDirIterator it(dirPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        it.next();
        QString manifestPath = it.filePath() + "/manifest.json";
        if (QFile::exists(manifestPath)) {
            auto manifest = ToolManifest::fromFile(manifestPath);
            if (manifest.has_value() && manifest->isValid()) {
                tools_.append(manifest.value());
            } else {
                qWarning() << "Skipping invalid manifest:" << manifestPath;
            }
        }
    }

    // Also check for manifest.json directly in the search path
    if (QFile::exists(dirPath + "/manifest.json")) {
        auto manifest = ToolManifest::fromFile(dirPath + "/manifest.json");
        if (manifest.has_value() && manifest->isValid()) {
            // Only add if not already added from subdirectory scan
            bool exists = false;
            for (const auto &t : tools_) {
                if (t.name == manifest->name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                tools_.append(manifest.value());
            }
        }
    }
}

const ToolManifest *ToolRegistry::findByName(const QString &name) const
{
    for (const auto &tool : tools_) {
        if (tool.name == name) {
            return &tool;
        }
    }
    return nullptr;
}

QMap<QString, QVector<ToolManifest>> ToolRegistry::toolsByCategory() const
{
    QMap<QString, QVector<ToolManifest>> grouped;
    for (const auto &tool : tools_) {
        grouped[tool.category].append(tool);
    }
    return grouped;
}

QStringList ToolRegistry::categories() const
{
    QSet<QString> cats;
    for (const auto &tool : tools_) {
        cats.insert(tool.category);
    }
    QStringList sorted = cats.values();
    sorted.sort(Qt::CaseInsensitive);
    return sorted;
}
