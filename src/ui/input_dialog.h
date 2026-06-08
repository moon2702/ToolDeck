#ifndef INPUT_DIALOG_H
#define INPUT_DIALOG_H

#include <QDialog>
#include <QHash>
#include <QVBoxLayout>
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

/// Dynamically-built form dialog for tool parameter input.
/// Reads ToolInputDef definitions and creates appropriate Qt widgets.
class InputDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param manifest        The tool manifest with inputs[] definitions
    /// @param parent          Parent widget
    explicit InputDialog(const ToolManifest &manifest, QWidget *parent = nullptr);

    /// Return the collected input values (name → value string)
    QHash<QString, QString> values() const;

private slots:
    void onValueChanged();
    void onBrowseFile();
    void onBrowseDir();

private:
    void setupUi();
    void buildForm();
    void updatePreview();

    const ToolManifest &manifest_;

    // Layout
    QVBoxLayout *mainLayout_;
    QFormLayout *formLayout_;
    QLabel *previewLabel_;
    QLabel *errorLabel_;
    QDialogButtonBox *buttonBox_;

    // Widget tracking: inputName → widget
    struct FieldWidgets {
        QWidget *container = nullptr;   // The main input widget
        QPushButton *browseBtn = nullptr; // For file/dir types
    };
    QHash<QString, FieldWidgets> fields_;
};

#endif // INPUT_DIALOG_H
