#ifndef TOOL_MANIFEST_H
#define TOOL_MANIFEST_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVector>

/// Run mode for a tool
enum class RunMode {
    Oneshot,    // Run once and exit
    Daemon,     // Long-running background process
    Scheduled   // Periodic execution
};

/// Output display mode
enum class OutputMode {
    Stream,     // Real-time stdout streaming
    Result,     // Show final output only
    Status      // Show only exit code / status indicator
};

/// Input type for form field generation
enum class InputType {
    Text,       // QLineEdit — free text
    File,       // QLineEdit + browse button → QFileDialog::getOpenFileName
    Dir,        // QLineEdit + browse button → QFileDialog::getExistingDirectory
    Choice,     // QComboBox — pick from predefined options
    Bool,       // QCheckBox — on/off flag
    Int,        // QSpinBox — integer
    Number      // QDoubleSpinBox — float
};

/// Definition of a single tool input parameter.
/// The platform uses this to auto-generate a form dialog.
struct ToolInputDef {
    QString name;           // Variable name, used in argTemplate
    QString label;          // Display label in the form
    QString description;    // Help text shown below the widget
    InputType type = InputType::Text;
    bool required = false;
    QString defaultValue;   // Pre-filled value
    QStringList choices;    // For Choice type: list of options
    int intMin = 0;         // For Int type: minimum value
    int intMax = 99999;     // For Int type: maximum value
    double numMin = 0.0;    // For Number type
    double numMax = 99999.0;
    double numDecimals = 2; // For Number type
};

/// Parsed tool manifest data
struct ToolManifest {
    QString name;           // Unique tool ID (directory name)
    QString displayName;    // Human-readable name
    QString description;    // Tool description
    QString version;        // Semantic version
    QString category;       // Grouping category
    QString icon;           // Freedesktop icon name or path
    QString command;        // Executable or interpreter path
    QStringList args;       // Command arguments
    QString workingDir;     // Working directory for execution
    RunMode runMode = RunMode::Oneshot;
    OutputMode outputMode = OutputMode::Stream;
    QJsonObject inputSchema;    // Optional JSON Schema for tool inputs (legacy)
    QVector<ToolInputDef> inputs; // Structured input definitions for form generation
    QString argTemplate;         // Template string: "-a {algo} {verbose:--v} {file}"
    QStringList os;              // Compatible OS list (empty = all platforms): ["windows","linux","macos"]

    /// Directory containing this manifest
    QString directoryPath;

    /// Parse a manifest from a JSON file path. Returns nullopt on failure.
    static std::optional<ToolManifest> fromFile(const QString &path);

    /// Parse a manifest from JSON bytes
    static std::optional<ToolManifest> fromJson(const QByteArray &data, const QString &sourcePath = {});

    /// Validate the manifest has all required fields
    bool isValid() const;

    /// Build the final argument list by expanding argTemplate with input values
    QStringList buildArgs(const QHash<QString, QString> &inputValues) const;

private:
    static RunMode parseRunMode(const QString &str);
    static OutputMode parseOutputMode(const QString &str);
    static InputType parseInputType(const QString &str);
    static QVector<ToolInputDef> parseInputs(const QJsonArray &arr);
};

#endif // TOOL_MANIFEST_H
