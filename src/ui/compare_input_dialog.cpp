#include "compare_input_dialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFont>

CompareInputDialog::CompareInputDialog(const ToolManifest &manifest, QWidget *parent)
    : QDialog(parent)
    , manifest_(manifest)
{
    setupUi();
    updatePreviews();
}

void CompareInputDialog::setupUi()
{
    setWindowTitle(QString("并排对比 — %1").arg(manifest_.displayName));
    setMinimumWidth(760);
    setMinimumHeight(400);

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(10);

    // Description
    if (!manifest_.description.isEmpty()) {
        descLabel_ = new QLabel(manifest_.description, this);
        descLabel_->setWordWrap(true);
        descLabel_->setStyleSheet("color: #555; font-size: 12px;");
        mainLayout_->addWidget(descLabel_);
    }

    // ---- Side-by-side groups ----
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // Left group
    leftGroup_ = new QGroupBox("参数组 A", this);
    leftForm_ = new QFormLayout(leftGroup_);
    leftForm_->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    leftForm_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    leftForm_->setHorizontalSpacing(8);
    leftForm_->setVerticalSpacing(8);
    buildForm(leftGroup_, leftForm_, leftFields_, leftBrowseBtns_);
    splitter->addWidget(leftGroup_);

    // Right group
    rightGroup_ = new QGroupBox("参数组 B", this);
    rightForm_ = new QFormLayout(rightGroup_);
    rightForm_->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    rightForm_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    rightForm_->setHorizontalSpacing(8);
    rightForm_->setVerticalSpacing(8);
    buildForm(rightGroup_, rightForm_, rightFields_, rightBrowseBtns_);
    splitter->addWidget(rightGroup_);

    splitter->setSizes({380, 380});
    mainLayout_->addWidget(splitter);

    // ---- Command previews ----
    auto *previewRow = new QHBoxLayout();

    leftPreview_ = new QLabel(this);
    leftPreview_->setWordWrap(true);
    leftPreview_->setStyleSheet(
        "background-color: #2d2d2d; color: #8bc34a; padding: 6px;"
        "border-radius: 4px; font-family: monospace; font-size: 11px;");
    previewRow->addWidget(leftPreview_);

    rightPreview_ = new QLabel(this);
    rightPreview_->setWordWrap(true);
    rightPreview_->setStyleSheet(
        "background-color: #2d2d2d; color: #8bc34a; padding: 6px;"
        "border-radius: 4px; font-family: monospace; font-size: 11px;");
    previewRow->addWidget(rightPreview_);

    mainLayout_->addLayout(previewRow);

    // ---- Buttons ----
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox_->button(QDialogButtonBox::Ok)->setText("运行对比");
    buttonBox_->button(QDialogButtonBox::Cancel)->setText("取消");
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout_->addWidget(buttonBox_);
}

void CompareInputDialog::buildForm(QGroupBox *group, QFormLayout *form,
                                     QHash<QString, QWidget *> &fields,
                                     QHash<QString, QPushButton *> &browseBtns)
{
    Q_UNUSED(group);

    for (const auto &def : manifest_.inputs) {
        QWidget *widget = nullptr;
        QPushButton *browseBtn = nullptr;

        switch (def.type) {
        case InputType::Text: {
            auto *edit = new QLineEdit(this);
            edit->setText(def.defaultValue);
            edit->setPlaceholderText(def.description.isEmpty() ? def.label : def.description);
            connect(edit, &QLineEdit::textChanged, this, &CompareInputDialog::onValueChanged);
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
            connect(edit, &QLineEdit::textChanged, this, &CompareInputDialog::onValueChanged);
            hlay->addWidget(edit);
            browseBtn = new QPushButton("浏览", container);
            browseBtn->setFixedWidth(50);
            browseBtn->setProperty("inputName", def.name);
            connect(browseBtn, &QPushButton::clicked, this, [edit]() {
                QString path = QFileDialog::getOpenFileName(nullptr, "选择文件", edit->text());
                if (!path.isEmpty()) edit->setText(path);
            });
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
            connect(edit, &QLineEdit::textChanged, this, &CompareInputDialog::onValueChanged);
            hlay->addWidget(edit);
            browseBtn = new QPushButton("浏览", container);
            browseBtn->setFixedWidth(50);
            connect(browseBtn, &QPushButton::clicked, this, [edit]() {
                QString path = QFileDialog::getExistingDirectory(nullptr, "选择目录", edit->text());
                if (!path.isEmpty()) edit->setText(path);
            });
            hlay->addWidget(browseBtn);
            widget = container;
            break;
        }
        case InputType::Choice: {
            auto *combo = new QComboBox(this);
            for (const auto &c : def.choices) combo->addItem(c);
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
            if (def.defaultValue == "true" || def.defaultValue == "1")
                check->setChecked(true);
            connect(check, &QCheckBox::toggled, this, [this](bool) { onValueChanged(); });
            widget = check;
            // Bool uses checkbox text as label — description shown underneath
            QString lbl = def.label;
            if (!def.description.isEmpty())
                lbl += QString("  <span style='color:#888;font-size:11px;'>%1</span>").arg(def.description);
            auto *bl = new QLabel(lbl, this);
            bl->setTextFormat(Qt::RichText);
            form->addRow(bl, widget);
            fields[def.name] = widget;
            if (browseBtn) browseBtns[def.name] = browseBtn;
            continue; // skip normal addRow
        }
        case InputType::Int: {
            auto *spin = new QSpinBox(this);
            spin->setRange(def.intMin, def.intMax);
            if (!def.defaultValue.isEmpty()) spin->setValue(def.defaultValue.toInt());
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this](int) { onValueChanged(); });
            widget = spin;
            break;
        }
        case InputType::Number: {
            auto *spin = new QDoubleSpinBox(this);
            spin->setRange(def.numMin, def.numMax);
            spin->setDecimals(def.numDecimals);
            if (!def.defaultValue.isEmpty()) spin->setValue(def.defaultValue.toDouble());
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, [this](double) { onValueChanged(); });
            widget = spin;
            break;
        }
        }

        // Label
        QString labelText = def.label;
        if (def.required) labelText += " <span style='color:#F44336;'>*</span>";
        auto *fieldLabel = new QLabel(labelText, this);
        fieldLabel->setTextFormat(Qt::RichText);
        if (!def.description.isEmpty() && def.type != InputType::Bool)
            fieldLabel->setToolTip(def.description);

        form->addRow(fieldLabel, widget);
        fields[def.name] = widget;
        if (browseBtn) browseBtns[def.name] = browseBtn;
    }
}

QHash<QString, QString> CompareInputDialog::leftValues() const
{
    QHash<QString, QString> out;
    collectValues(leftFields_, out);
    return out;
}

QHash<QString, QString> CompareInputDialog::rightValues() const
{
    QHash<QString, QString> out;
    collectValues(rightFields_, out);
    return out;
}

void CompareInputDialog::collectValues(const QHash<QString, QWidget *> &fields,
                                         QHash<QString, QString> &out) const
{
    for (const auto &def : manifest_.inputs) {
        QWidget *w = fields.value(def.name);
        if (!w) continue;
        QString val;

        switch (def.type) {
        case InputType::Text:
        case InputType::File:
        case InputType::Dir: {
            QLineEdit *edit = nullptr;
            if (def.type == InputType::File || def.type == InputType::Dir)
                edit = w->findChild<QLineEdit *>();
            else
                edit = qobject_cast<QLineEdit *>(w);
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

        if (!val.isEmpty()) out[def.name] = val;
    }
}

void CompareInputDialog::onValueChanged()
{
    updatePreviews();
}

void CompareInputDialog::updatePreviews()
{
    QHash<QString, QString> lv = leftValues();
    QHash<QString, QString> rv = rightValues();

    QStringList la = manifest_.buildArgs(lv);
    QStringList ra = manifest_.buildArgs(rv);

    auto fmt = [](const QStringList &args) -> QString {
        if (args.isEmpty()) return "<span style='color:#888;'>填写参数后预览</span>";
        return "<b>A:</b> " + args.join(' ');
    };

    auto fmtR = [](const QStringList &args) -> QString {
        if (args.isEmpty()) return "<span style='color:#888;'>填写参数后预览</span>";
        return "<b>B:</b> " + args.join(' ');
    };

    leftPreview_->setText(fmt(la));
    rightPreview_->setText(fmtR(ra));
}
