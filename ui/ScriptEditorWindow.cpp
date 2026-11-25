#include "ScriptEditorWindow.h"
#include "ScriptVocabulary.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QTextBlock>
#include <QFont>

ScriptEditorWindow::ScriptEditorWindow(QWidget* parent)
    : QMainWindow(parent), m_highlighter(nullptr), m_isModified(false), m_suggestionPrefixStart(-1)
{
    setWindowTitle("Script Editor");
    resize(900, 650);

    setupUI();
    setupMenus();
    setupToolbar();
    setupStatusBar();
    updateTitle();
}

void ScriptEditorWindow::setupUI()
{
    m_editor = new QPlainTextEdit(this);
    QFont mono("Monospace");
    mono.setStyleHint(QFont::TypeWriter);
    m_editor->setFont(mono);
    m_editor->installEventFilter(this);
    connect(m_editor, &QPlainTextEdit::textChanged, this, &ScriptEditorWindow::onTextChanged);
    setCentralWidget(m_editor);

    // Qt::Popup implicitly takes an X11 mouse/keyboard grab while visible.
    // Re-showing it on every keystroke (updateSuggestions() runs on every
    // textChanged) can leave that grab stuck under some window managers,
    // freezing input to the whole app until something forces a WM-level
    // focus change (e.g. the Super key) - reported as "the program freezes".
    // A Qt::Tool window with WA_ShowWithoutActivating never grabs input and
    // never steals focus from m_editor, avoiding the problem entirely; we
    // lose the automatic "click outside closes it" behavior Popup gave us
    // for free, so that's now handled explicitly via FocusOut below.
    m_suggestionBox = new QListWidget(this);
    m_suggestionBox->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_suggestionBox->setAttribute(Qt::WA_ShowWithoutActivating);
    m_suggestionBox->setFocusPolicy(Qt::NoFocus);
    m_suggestionBox->setMaximumHeight(180);
}

void ScriptEditorWindow::setupMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &ScriptEditorWindow::onOpen);

    fileMenu->addSeparator();

    QAction* saveAction = fileMenu->addAction(tr("&Save"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &ScriptEditorWindow::onSave);

    QAction* saveAsAction = fileMenu->addAction(tr("Save &As..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &ScriptEditorWindow::onSaveAs);

    fileMenu->addSeparator();

    QAction* closeAction = fileMenu->addAction(tr("&Close"));
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
}

void ScriptEditorWindow::setupToolbar()
{
    m_toolBar = addToolBar(tr("Script Toolbar"));
    m_toolBar->setMovable(false);

    QAction* openAction = m_toolBar->addAction("Open");
    connect(openAction, &QAction::triggered, this, &ScriptEditorWindow::onOpen);

    QAction* saveAction = m_toolBar->addAction("Save");
    connect(saveAction, &QAction::triggered, this, &ScriptEditorWindow::onSave);
}

void ScriptEditorWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready - Open a .cfg/.ai/.oms/.txt file", this);
    statusBar()->addWidget(m_statusLabel);
}

void ScriptEditorWindow::updateTitle()
{
    QString title = "Script Editor";
    if (!m_currentFilePath.isEmpty())
    {
        title += " - " + QFileInfo(m_currentFilePath).fileName();
    }
    if (m_isModified)
    {
        title = "*" + title;
    }
    setWindowTitle(title);
}

void ScriptEditorWindow::openFile(const QString& filePath)
{
    if (!maybeSave())
    {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot open:\n%1").arg(filePath));
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    m_editor->blockSignals(true);
    m_editor->setPlainText(QString::fromUtf8(data));
    m_editor->blockSignals(false);

    delete m_highlighter;
    m_highlighter = new ScriptHighlighter(ScriptHighlighter::modeForFileName(filePath), m_editor->document());

    m_currentFilePath = filePath;
    m_isModified = false;
    updateTitle();
    m_statusLabel->setText(QString("Loaded: %1").arg(filePath));
}

bool ScriptEditorWindow::saveTo(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot write to:\n%1").arg(filePath));
        return false;
    }
    file.write(m_editor->toPlainText().toUtf8());
    file.close();
    return true;
}

void ScriptEditorWindow::onOpen()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Script"), QString(),
        tr("Script files (*.cfg *.ai *.oms *.txt);;All Files (*)"));
    if (!filePath.isEmpty())
    {
        openFile(filePath);
    }
}

void ScriptEditorWindow::onSave()
{
    if (m_currentFilePath.isEmpty())
    {
        onSaveAs();
        return;
    }

    if (saveTo(m_currentFilePath))
    {
        m_isModified = false;
        updateTitle();
        m_statusLabel->setText(QString("Saved: %1").arg(m_currentFilePath));
    }
}

void ScriptEditorWindow::onSaveAs()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Script As"),
        m_currentFilePath.isEmpty() ? QDir::homePath() : m_currentFilePath,
        tr("Script files (*.cfg *.ai *.oms *.txt);;All Files (*)"));
    if (filePath.isEmpty())
    {
        return;
    }

    if (saveTo(filePath))
    {
        m_currentFilePath = filePath;

        delete m_highlighter;
        m_highlighter = new ScriptHighlighter(ScriptHighlighter::modeForFileName(filePath), m_editor->document());

        m_isModified = false;
        updateTitle();
        m_statusLabel->setText(QString("Saved: %1").arg(filePath));
    }
}

void ScriptEditorWindow::onTextChanged()
{
    m_isModified = true;
    updateTitle();
    updateSuggestions();
}

void ScriptEditorWindow::updateSuggestions()
{
    QTextCursor cursor = m_editor->textCursor();
    int posInBlock = cursor.positionInBlock();
    QString blockText = cursor.block().text();

    int start = posInBlock;
    while (start > 0 && (blockText[start - 1].isLetterOrNumber() || blockText[start - 1] == '_'))
    {
        start--;
    }
    QString prefix = blockText.mid(start, posInBlock - start);

    if (prefix.length() < 2)
    {
        m_suggestionBox->hide();
        return;
    }

    const QStringList& vocab = (m_highlighter && ScriptHighlighter::modeForFileName(m_currentFilePath) == ScriptHighlighter::Mode::Tabular)
        ? ScriptVocabulary::tabularKeys()
        : ScriptVocabulary::callSyntaxFunctions();

    QStringList matches;
    for (const QString& word : vocab)
    {
        if (word.startsWith(prefix, Qt::CaseInsensitive) && word.compare(prefix, Qt::CaseInsensitive) != 0)
        {
            matches << word;
        }
        if (matches.size() >= 15)
        {
            break;
        }
    }

    if (matches.isEmpty())
    {
        m_suggestionBox->hide();
        return;
    }

    m_suggestionPrefixStart = cursor.block().position() + start;

    m_suggestionBox->clear();
    m_suggestionBox->addItems(matches);
    m_suggestionBox->setCurrentRow(0);

    QPoint below = m_editor->viewport()->mapToGlobal(m_editor->cursorRect().bottomLeft());
    m_suggestionBox->move(below);
    m_suggestionBox->resize(qMax(220, m_suggestionBox->sizeHintForColumn(0) + 24), qMin(180, matches.size() * 20 + 8));
    m_suggestionBox->show();
}

void ScriptEditorWindow::applySuggestion(const QString& word)
{
    if (m_suggestionPrefixStart < 0)
    {
        return;
    }

    QTextCursor cursor = m_editor->textCursor();
    cursor.setPosition(m_suggestionPrefixStart);
    cursor.setPosition(m_editor->textCursor().position(), QTextCursor::KeepAnchor);
    cursor.insertText(word);

    m_suggestionBox->hide();
    m_suggestionPrefixStart = -1;
}

bool ScriptEditorWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_editor && event->type() == QEvent::KeyPress && m_suggestionBox->isVisible())
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        switch (keyEvent->key())
        {
        case Qt::Key_Down:
        {
            int row = qMin(m_suggestionBox->currentRow() + 1, m_suggestionBox->count() - 1);
            m_suggestionBox->setCurrentRow(row);
            return true;
        }
        case Qt::Key_Up:
        {
            int row = qMax(m_suggestionBox->currentRow() - 1, 0);
            m_suggestionBox->setCurrentRow(row);
            return true;
        }
        case Qt::Key_Tab:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
            QListWidgetItem* item = m_suggestionBox->currentItem();
            if (item)
            {
                applySuggestion(item->text());
            }
            return true;
        }
        case Qt::Key_Escape:
            m_suggestionBox->hide();
            return true;
        default:
            break;
        }
    }

    // Qt::Popup used to close the suggestion box automatically on outside
    // clicks / focus loss - now that it's a non-grabbing Qt::Tool window
    // (see setupUI()), that has to be done explicitly.
    if (watched == m_editor && event->type() == QEvent::FocusOut)
    {
        m_suggestionBox->hide();
    }

    return QMainWindow::eventFilter(watched, event);
}

bool ScriptEditorWindow::maybeSave()
{
    if (!m_isModified)
    {
        return true;
    }

    QMessageBox::StandardButton ret = QMessageBox::warning(this, "Script Editor",
        "The script has been modified.\nDo you want to save your changes?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
    {
        onSave();
        return !m_isModified;
    }
    if (ret == QMessageBox::Cancel)
    {
        return false;
    }
    return true;
}

void ScriptEditorWindow::closeEvent(QCloseEvent* event)
{
    if (maybeSave())
    {
        m_suggestionBox->hide();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}
