#include "tool_manifest.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>

std::optional<ToolManifest> ToolManifest::fromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open manifest:" << path;
        return std::nullopt;
    }

    QByteArray data = file.readAll();
    file.close();

    auto manifest = fromJson(data, path);
    if (manifest.has_value()) {
        manifest->directoryPath = QFileInfo(path).absoluteDir().absolutePath();
    }
    return manifest;
}

std::optional<ToolManifest> ToolManifest::fromJson(const QByteArray &data, const QString &sourcePath)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in" << sourcePath << ":" << error.errorString();
        return std::nullopt;
    }

    if (!doc.isObject()) {
        qWarning() << "Manifest is not a JSON object:" << sourcePath;
        return std::nullopt;
    }

    QJsonObject obj = doc.object();
    ToolManifest m;

    m.name = obj.value("name").toString();
    m.displayName = obj.value("displayName").toString();
    m.description = obj.value("description").toString();
    m.version = obj.value("version").toString("1.0.0");
    m.category = obj.value("category").toString("custom");
    m.icon = obj.value("icon").toString("application-x-executable");
    m.command = obj.value("command").toString();
    m.workingDir = obj.value("workingDir").toString(".");

    // Parse args array
    QJsonArray argsArray = obj.value("args").toArray();
    for (const auto &arg : argsArray) {
        m.args.append(arg.toString());
    }

    m.runMode = parseRunMode(obj.value("runMode").toString("oneshot"));
    m.outputMode = parseOutputMode(obj.value("outputMode").toString("stream"));
    m.inputSchema = obj.value("inputSchema").toObject();

    // Parse structured inputs array
    m.inputs = parseInputs(obj.value("inputs").toArray());

    // Parse arg template
    m.argTemplate = obj.value("argTemplate").toString();

    return m;
}

bool ToolManifest::isValid() const
{
    return !name.isEmpty()
        && !displayName.isEmpty()
        && !command.isEmpty();
}

RunMode ToolManifest::parseRunMode(const QString &str)
{
    QString s = str.trimmed().toLower();
    if (s == "daemon")    return RunMode::Daemon;
    if (s == "scheduled") return RunMode::Scheduled;
    return RunMode::Oneshot;
}

OutputMode ToolManifest::parseOutputMode(const QString &str)
{
    QString s = str.trimmed().toLower();
    if (s == "result") return OutputMode::Result;
    if (s == "status") return OutputMode::Status;
    return OutputMode::Stream;
}

InputType ToolManifest::parseInputType(const QString &str)
{
    QString s = str.trimmed().toLower();
    if (s == "file")   return InputType::File;
    if (s == "dir")    return InputType::Dir;
    if (s == "choice") return InputType::Choice;
    if (s == "bool")   return InputType::Bool;
    if (s == "int")    return InputType::Int;
    if (s == "number") return InputType::Number;
    return InputType::Text;
}

QVector<ToolInputDef> ToolManifest::parseInputs(const QJsonArray &arr)
{
    QVector<ToolInputDef> result;
    for (const auto &val : arr) {
        if (!val.isObject()) continue;
        QJsonObject o = val.toObject();

        ToolInputDef def;
        def.name        = o.value("name").toString();
        def.label       = o.value("label").toString();
        def.description = o.value("description").toString();
        def.type        = parseInputType(o.value("type").toString("text"));
        def.required    = o.value("required").toBool(false);
        def.defaultValue = o.value("default").toString();

        // Parse choices array
        QJsonArray choicesArr = o.value("choices").toArray();
        for (const auto &c : choicesArr) {
            def.choices.append(c.toString());
        }

        def.intMin   = o.value("min").toInt(0);
        def.intMax   = o.value("max").toInt(99999);
        def.numMin   = o.value("min").toDouble(0.0);
        def.numMax   = o.value("max").toDouble(99999.0);
        def.numDecimals = o.value("decimals").toInt(2);

        if (def.name.isEmpty()) {
            qWarning() << "Skipping input def with no name";
            continue;
        }

        result.append(def);
    }
    return result;
}

QStringList ToolManifest::buildArgs(const QHash<QString, QString> &inputValues) const
{
    // If no arg template, just append all values in input definition order
    if (argTemplate.isEmpty()) {
        QStringList result = args;
        for (const auto &def : inputs) {
            QString val = inputValues.value(def.name);
            if (!val.isEmpty()) {
                result.append(val);
            }
        }
        return result;
    }

    // Parse the template and expand placeholders
    QString expanded = argTemplate;
    static QRegularExpression re(R"(\{([^}:]+?)(?::([^}]+))?\})");
    // Matches: {name}, {name:--flag}, {name:+v}, {-prefix name}

    QRegularExpressionMatchIterator it = re.globalMatch(argTemplate);
    // Collect matches and build result
    struct MatchInfo {
        qsizetype pos;
        qsizetype len;
        QString fullMatch;
        QString name;
        QString modifier;
    };
    QVector<MatchInfo> matches;
    int idx = 0;
    while (it.hasNext()) {
        auto m = it.next();
        matches.append({m.capturedStart(), m.capturedLength(),
                        m.captured(0), m.captured(1), m.captured(2)});
        Q_UNUSED(idx);
    }

    // Process matches in reverse to preserve positions
    for (int i = matches.size() - 1; i >= 0; --i) {
        const auto &m = matches[i];
        QString name = m.name;
        QString modifier = m.modifier;
        QString value = inputValues.value(name);
        QString replacement;

        if (modifier.startsWith("--") || modifier.startsWith('-')) {
            // Bool flag: {verbose:--verbose} — emit modifier only if value is "true"
            if (!value.isEmpty() && value != "false" && value != "0") {
                // For bool, just emit the flag
                if (!inputValues.contains(name)) {
                    replacement = modifier;
                } else {
                    replacement = modifier;
                }
            }
        } else if (modifier.startsWith("+")) {
            // Short flag: {verbose:+v} — emit modifier only if value is "true"
            if (value == "true" || value == "1") {
                replacement = modifier;
            }
        } else {
            // Plain value or prefix: {name} or {-o name}
            replacement = value;
        }

        expanded.replace(m.pos, m.len, replacement);
    }

    // Clean up extra whitespace
    expanded = expanded.simplified();

    // Split into args list
    QStringList result = args;
    if (!expanded.isEmpty()) {
        // Split by whitespace, respecting that we already processed the template
        result.append(expanded.split(' ', Qt::SkipEmptyParts));
    }

    return result;
}
