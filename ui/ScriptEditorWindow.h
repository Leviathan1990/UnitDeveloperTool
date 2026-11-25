#ifndef SCRIPTEDITORWINDOW_H
#define SCRIPTEDITORWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QToolBar>

#include "ScriptHighlighter.h"

// A syntax-highlighting editor for the Outforce text-script family
// (.cfg/.oms call-syntax, .ai tabular) with a live autocomplete popup
// ("suggestion box"), sourced from ScriptVocabulary.h's ground-truthed
// keyword lists - see that header for where the vocabulary comes from and
// why it replaces (rather than ports) BoxBuilder's own broken/externally-
// dependent highlighter+suggestion feature.
//
// Structurally follows the same pattern as EffectsEditorWindow/
// BoxArchiveWindow (own QMainWindow, lazily constructed + reused,
// WA_DeleteOnClose off). Usable both standalone (its own File > Open) and
// driven externally via openFile() (BoxArchiveWindow hands it a temp path
// extracted from an open .box archive, the same way it already does for
// PackedProject.OPF -> MainWindow).
class ScriptEditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ScriptEditorWindow(QWidget* parent = nullptr);

    void openFile(const QString& filePath);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onOpen();
    void onSave();
    void onSaveAs();
    void onTextChanged();

private:
    QPlainTextEdit* m_editor;
    ScriptHighlighter* m_highlighter;
    QListWidget* m_suggestionBox; // Qt::Popup, positioned under the cursor
    QLabel* m_statusLabel;
    QToolBar* m_toolBar;

    QString m_currentFilePath;
    bool m_isModified;
    int m_suggestionPrefixStart; // absolute document position where the typed prefix begins

    void setupUI();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();
    void updateTitle();
    bool saveTo(const QString& filePath);
    bool maybeSave();

    void updateSuggestions();
    void applySuggestion(const QString& word);
};

#endif // SCRIPTEDITORWINDOW_H
