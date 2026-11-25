#ifndef BOXARCHIVEWINDOW_H
#define BOXARCHIVEWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QLabel>
#include <QToolBar>
#include <QMap>
#include <QVector>

#include "box/BoxStruct.h"

class MainWindow;
class ScriptEditorWindow;
class ImageInspectorWindow;

// Browses/extracts/rebuilds a .box archive, and hands its PackedProject.OPF
// entry (if present) straight into MainWindow's own existing OPF editor -
// see MainWindow::openFile() (now public for exactly this purpose). This is
// the integration point that actually ties the box-archive and OPF-editing
// halves of the tool together: opening a real Content.box and double-
// clicking PackedProject.OPF inside it lands you in the exact same
// AssetTreeWidget/AssetPreviewWidget/OpfWriter::Save flow as opening a
// stand-alone .opf file directly, with no separate program needed.
//
// Structurally follows the same pattern as EffectsEditorWindow/
// AIEditorWindow (own QMainWindow, lazily constructed + reused by
// MainWindow, WA_DeleteOnClose off) rather than being docked/tabbed into
// MainWindow itself.
class BoxArchiveWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BoxArchiveWindow(MainWindow* mainWindow, QWidget* parent = nullptr);

    // Open a .box archive directly (used both by the internal "Open..."
    // action and by MainWindow when launching this window from a menu
    // action that already has a path, e.g. a recent-files entry).
    void openFile(const QString& filePath);

private slots:
    void onOpen();
    void onNewFromFolder();
    void onAddFiles();
    void onExtractSelected();
    void onExtractAll();
    void onSaveArchive();
    void onSaveArchiveAs();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    MainWindow* m_mainWindow; // not owned - used to open an extracted OPF in the existing editor

    QVector<BoxFile> m_files;
    QString m_currentArchivePath;

    // Temp-file path -> index into m_files, for every entry handed off to a
    // sub-editor (MainWindow's OPF editor, ScriptEditorWindow, or
    // ImageInspectorWindow). Before rebuilding the archive, each tracked
    // temp file is re-read from disk (every sub-editor's own Save/Save As
    // always writes back to whatever path it was opened from - see
    // MainWindow::onSaveFile()'s writeBackup(m_currentFilePath, ...) for the
    // OPF case - so this picks up any edits made there) and substituted into
    // m_files.
    QMap<QString, int> m_extractedTempFiles;

    QTreeWidget* m_treeWidget;
    QLabel* m_statusLabel;
    QToolBar* m_toolBar;

    ScriptEditorWindow* m_scriptEditor = nullptr;
    ImageInspectorWindow* m_imageInspector = nullptr;

    void setupUI();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();

    void rebuildTree();
    void updateTitle();
    QTreeWidgetItem* findOrCreateFolder(QTreeWidgetItem* parent, const QString& name);
    void extractEntry(int index, const QString& destPath);
    void refreshExtractedEntries();
    bool saveTo(const QString& filePath);

    // Recursively walks rootDir and appends one BoxFile per file found,
    // named relative to rootDir with '/' separators (matching the naming
    // convention rebuildTree() already expects). Returns false (with a
    // message box) if rootDir contains no files at all.
    bool collectFilesFromFolder(const QString& rootDir, QVector<BoxFile>& outFiles);
};

#endif // BOXARCHIVEWINDOW_H
