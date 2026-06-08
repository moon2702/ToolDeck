#include "input_dialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QFont>

InputDialog::InputDialog(const ToolManifest &manifest, QWidget *parent)
    : QDialog(parent)
    , manifest_(manifest)
{
    setupUi();
    buildForm();
    updatePreview();
}

void InputDialog::setupUi()
{
    setWindowTitle(QString("运行工具 — %1").arg(manifest_.displayName));
    setMinimumWidth(480);
    setMaximumWidth(640);

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(12);

    // Description label
    if (!manifest_.description.isEmpty()) {
        auto *descLabel = new QLabel(manifest_.description, this);
        descLabel->setWordWrap(true);
        descLabel->setStyleSheet("color: #555; font-size: 12px; padding-bottom: 4px;");
        mainLayout_->addWidget(descLabel);
    }

    // Form area
    formLayout_ = new QFormLayout();
    formLayout_->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    formLayout_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout_->setHorizontalSpacing(12);
    formLayout_->setVerticalSpacing(10);
    mainLayout_->addLayout(formLayout_);

    // Error label (hidden by default)
    errorLabel_ = new QLabel(this);
    errorLabel_->setStyleSheet("color: #F44336; font-size: 12px; font-weight: bold;");
    errorLabel_->setVisible(false);
    mainLayout_->addWidget(errorLabel_);

    // Command preview
    previewLabel_ = new QLabel(this);
    previewLabel_->setWordWrap(true);
    previewLabel_->setStyleSheet(
        "background-color: #2d2d2d; color: #8bc34a; padding: 8px;"
        "border-radius: 4px; font-family: monospace; font-size: 12px;"
    );
    mainLayout_->addWidget(previewLabel_);

    // Buttons
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox_->button(QDialogButtonBox::Ok)->setText("运行");
    buttonBox_->button(QDialogButtonBox::Cancel)->setText("取消");
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout_->addWidget(buttonBox_);
}

void InputDialog::buildForm()
{
    for (const auto &def : manifest_.inputs) {
        QWidget *widget = nullptr;
        QPushButton *browseBtn = nullptr;

        switch (def.type) {
        case InputType::Text: {
            auto *edit = new QLineEdit(this);
            edit->setText(def.defaultValue);
            edit->setPlaceholderText(def.description.isEmpty() ? def.label : def.description);
            connect(edit, &QLineEdit::textChanged, this, &InputDialog::onValueChanged);
            widget = edit;
            break;
        }
        case InputType::File: {
            auto *container = new QWidget(this);
            auto *hlay = new QHBoxLayout(container);
            hlay->setContentsMargins(0, 0, 0, 0);
            hlay->setSpacing(4);

            auto *edit = new QLineEdit(container);
            edit->setText(def.defaultValue);
            edit->setPlaceholderText("选择文件...");
            connect(edit, &QLineEdit::textChanged, this, &InputDialog::onValueChanged);
            hlay->addWidget(edit);

            browseBtn = new QPushButton("浏览", container);
            browseBtn->setFixedWidth(60);
            browseBtn->setProperty("inputName", def.name);
            connect(browseBtn, &QPushButton::clicked, this, &InputDialog::onBrowseFile);
            hlay->addWidget(browseBtn);

            widget = container;
            break;
        }
        case InputType::Dir: {
            auto *container = new QWidget(this);
            auto *hlay = new QHBoxLayout(container);
            hlay->setContentsMargins(0, 0, 0, 0);
            hlay->setSpacing(4);

            auto *edit = new QLineEdit(container);
            edit->setText(def.defaultValue);
            edit->setPlaceholderText("选择目录...");
            connect(edit, &QLineEdit::textChanged, this, &InputDialog::onValueChanged);
            hlay->addWidget(edit);

            browseBtn = new QPushButton("浏览", container);
            browseBtn->setFixedWidth(60);
            browseBtn->setProperty("inputName", def.name);
            connect(browseBtn, &QPushButton::clicked, this, &InputDialog::onBrowseDir);
            hlay->addWidget(browseBtn);

            widget = container;
            break;
        }
        case InputType::Choice: {
            auto *combo = new QComboBox(this);
            for (const auto &choice : def.choices) {
                combo->addItem(choice);
            }
            if (!def.defaultValue.isEmpty()) {
                int idx = combo->findText(def.defaultValue);
                if (idx >= 0) combo->setCurrentIndex(idx);
            }
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int) { onValueChanged(); });
            widget = combo;
            break;
        }
        case InputType::Bool: {
            auto *check = new QCheckBox(def.label, this);
            if (def.defaultValue == "true" || def.defaultValue == "1") {
                check->setChecked(true);
            }
            connect(check, &QCheckBox::toggled, this, [this](bool) { onValueChanged(); });
            widget = check;

            // Bool uses label as checkbox text, so custom label via description
            QString labelText = def.label;
            if (!def.description.isEmpty()) {
                labelText += QString("  <span style='color:#888;font-size:11px;'>%1</span>").arg(def.description);
            }
            auto *boolLabel = new QLabel(labelText, this);
            boolLabel->setTextFormat(Qt::RichText);
            formLayout_->addRow(boolLabel, widget);
            fields_[def.name] = {widget, nullptr};
            continue; // Skip the normal addRow below — we handle label differently
        }
        case InputType::Int: {
            auto *spin = new QSpinBox(this);
            spin->setRange(def.intMin, def.intMax);
            if (!def.defaultValue.isEmpty()) {
                spin->setValue(def.defaultValue.toInt());
            }
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this](int) { onValueChanged(); });
            widget = spin;
            break;
        }
        case InputType::Number: {
            auto *spin = new QDoubleSpinBox(this);
            spin->setRange(def.numMin, def.numMax);
            spin->setDecimals(def.numDecimals);
            if (!def.defaultValue.isEmpty()) {
                spin->setValue(def.defaultValue.toDouble());
            }
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, [this](double) { onValueChanged(); });
            widget = spin;
            break;
        }
        }

        // Build label text
        QString labelText = def.label;
        if (def.required) {
            labelText += " <span style='color:#F44336;'>*</span>";
        }
        auto *fieldLabel = new QLabel(labelText, this);
        fieldLabel->setTextFormat(Qt::RichText);

        // Add description as tooltip or sub-label
        if (!def.description.isEmpty() && def.type != InputType::Bool) {
            fieldLabel->setToolTip(def.description);
        }

        formLayout_->addRow(fieldLabel, widget);
        fields_[def.name] = {widget, browseBtn};
    }

    // If no inputs defined, add a plain text entry as fallback
    if (manifest_.inputs.isEmpty()) {
        auto *fallbackEdit = new QLineEdit(this);
        fallbackEdit->setPlaceholderText("直接输入命令行参数，用空格分隔...");
        connect(fallbackEdit, &QLineEdit::textChanged, this, &InputDialog::onValueChanged);
        formLayout_->addRow("命令行参数:", fallbackEdit);
        fields_["_fallback_"] = {fallbackEdit, nullptr};
    }
}

QHash<QString, QString> InputDialog::values() const
{
    QHash<QString, QString> result;

    // Single fallback field (no structured inputs)
    if (manifest_.inputs.isEmpty()) {
        auto it = fields_.find("_fallback_");
        if (it != fields_.end()) {
            auto *edit = qobject_cast<QLineEdit *>(it->container);
            if (edit && !edit->text().trimmed().isEmpty()) {
                result["_fallback_"] = edit->text().trimmed();
            }
        }
        return result;
    }

    for (const auto &def : manifest_.inputs) {
        auto it = fields_.find(def.name);
        if (it == fields_.end()) continue;

        QWidget *w = it->container;
        QString val;

        switch (def.type) {
        case InputType::Text:
        case InputType::File:
        case InputType::Dir: {
            // File/Dir: the container is a QWidget with a QLineEdit inside
            QLineEdit *edit = nullptr;
            if (def.type == InputType::File || def.type == InputType::Dir) {
                edit = w->findChild<QLineEdit *>();
            } else {
                edit = qobject_cast<QLineEdit *>(w);
            }
            if (edit) val = edit->text().trimmed();
            break;
        }
        case InputType::Choice: {
            auto *combo = qobject_cast<QComboBox *>(w);
            if (combo) val = combo->currentText();
            break;
        }
        case InputType::Bool: {
            auto *check = qobject_cast<QCheckBox *>(w);
            if (check) val = check->isChecked() ? "true" : "false";
            break;
        }
        case InputType::Int: {
            auto *spin = qobject_cast<QSpinBox *>(w);
            if (spin) val = QString::number(spin->value());
            break;
        }
        case InputType::Number: {
            auto *spin = qobject_cast<QDoubleSpinBox *>(w);
            if (spin) val = QString::number(spin->value(), 'f', def.numDecimals);
            break;
        }
        }

        if (!val.isEmpty()) {
            result[def.name] = val;
        }
    }

    return result;
}

void InputDialog::onValueChanged()
{
    updatePreview();
}

void InputDialog::onBrowseFile()
{
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;

    QString inputName = btn->property("inputName").toString();
    auto it = fields_.find(inputName);
    if (it == fields_.end()) return;

    QLineEdit *edit = it->container->findChild<QLineEdit *>();
    QString startDir = edit ? edit->text() : QString();

    QString path = QFileDialog::getOpenFileName(this, "选择文件", startDir);
    if (!path.isEmpty() && edit) {
        edit->setText(path);
    }
}

void InputDialog::onBrowseDir()
{
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;

    QString inputName = btn->property("inputName").toString();
    auto it = fields_.find(inputName);
    if (it == fields_.end()) return;

    QLineEdit *edit = it->container->findChild<QLineEdit *>();
    QString startDir = edit ? edit->text() : QString();

    QString path = QFileDialog::getExistingDirectory(this, "选择目录", startDir);
    if (!path.isEmpty() && edit) {
        edit->setText(path);
    }
}

void InputDialog::updatePreview()
{
    QHash<QString, QString> vals = values();

    // If fallback mode, show the raw text
    if (manifest_.inputs.isEmpty()) {
        QString raw = vals.value("_fallback_");
        if (raw.isEmpty()) {
            previewLabel_->setText("<span style='color:#888;'>输入参数后将在此显示命令预览</span>");
        } else {
            previewLabel_->setText(QString("<b>命令:</b> %1 %2")
                .arg(manifest_.command)
                .arg(raw));
        }
        return;
    }

    // Build arg list via the manifest's template engine
    QStringList finalArgs = manifest_.buildArgs(vals);

    // Build display string
    QString cmdPreview = manifest_.command;
    for (const auto &a : finalArgs) {
        cmdPreview += " " + a;
    }

    if (finalArgs.isEmpty()) {
        previewLabel_->setText("<span style='color:#888;'>填写参数后将在此显示命令预览</span>");
    } else {
        previewLabel_->setText(QString("<b>命令预览:</b> %1").arg(cmdPreview));
    }
}
