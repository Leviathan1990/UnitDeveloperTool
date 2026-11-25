#ifndef IMAGEINSPECTORWINDOW_H
#define IMAGEINSPECTORWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QToolBar>
#include <QImage>

#include "box/ShortcutManager.h"

// A small image viewer/editor for images extracted from a .box archive -
// only the subset of the original BoxBuilder ImageInspector's toolbar
// actions that were actually wired up and working there (rotate 90°,
// mirror horizontal/vertical, resize, save/export) - several buttons in
// the original (X_90/X_180/X_270, a second 90° action) were present in its
// .ui but never connected to any handler, confirmed dead during
// exploration, and are not reproduced here.
//
// Structurally follows the same pattern as ScriptEditorWindow/
// EffectsEditorWindow (own QMainWindow, lazily constructed + reused).
class ImageInspectorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ImageInspectorWindow(QWidget* parent = nullptr);

    void openFile(const QString& filePath);

private slots:
    void onSave();
    void onSaveAs();
    void onRotate();
    void onMirrorHorizontal();
    void onMirrorVertical();
    void onCenter();
    void onSizeFieldChanged();

private:
    QLabel* m_imageLabel;
    QSpinBox* m_widthSpin;
    QSpinBox* m_heightSpin;
    QCheckBox* m_keepAspectCheckbox;
    QComboBox* m_formatCombo;
    QLabel* m_statusLabel;
    QToolBar* m_toolBar;

    ShortcutManager* m_shortcutManager;

    QImage m_image; // live, edited image (rotate/mirror applied in place)
    QString m_currentFilePath;
    bool m_centered;
    bool m_updatingSizeFields; // guard against onSizeFieldChanged feedback while we set values programmatically

    void setupUI();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();
    void updateTitle();
    void renderImage();
    void updateSizeFieldsFromImage();
    bool saveTo(const QString& filePath);
};

#endif // IMAGEINSPECTORWINDOW_H
