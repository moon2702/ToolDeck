#ifndef COMPARE_INPUT_DIALOG_H
#define COMPARE_INPUT_DIALOG_H

#include <QDialog>
#include <QHash>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>

#include "core/tool_manifest.h"

class QLineEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;

/// Dual-input form for compare mode.
/// Two identical QGroupBox columns (A / B) built from ToolInputDef[].
/// Returns left and right parameter sets independently.
class CompareInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CompareInputDialog(const ToolManifest &manifest, QWidget *parent = nullptr);

    /// Return collected values for left panel (param name → value string)
    QHash<QString, QString> leftValues() const;

    /// Return collected values for right panel
    QHash<QString, QString> rightValues() const;

private slots:
    void onValueChanged();

private:
    void setupUi();
    void buildForm(QGroupBox *group, QFormLayout *form,
                   QHash<QString, QWidget *> &fields,
                   QHash<QString, QPushButton *> &browseBtns);
    void collectValues(const QHash<QString, QWidget *> &fields,
                       QHash<QString, QString> &out) const;
    void updatePreviews();

    const ToolManifest &manifest_;

    // Layout
    QVBoxLayout *mainLayout_;
    QLabel *descLabel_;
    QGroupBox *leftGroup_;
    QGroupBox *rightGroup_;
    QFormLayout *leftForm_;
    QFormLayout *rightForm_;
    QLabel *leftPreview_;
    QLabel *rightPreview_;
    QDialogButtonBox *buttonBox_;

    // Field tracking per side
    QHash<QString, QWidget *> leftFields_;
    QHash<QString, QWidget *> rightFields_;
    QHash<QString, QPushButton *> leftBrowseBtns_;
    QHash<QString, QPushButton *> rightBrowseBtns_;
};

#endif // COMPARE_INPUT_DIALOG_H
