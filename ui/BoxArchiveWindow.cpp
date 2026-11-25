#include "BoxArchiveWindow.h"
#include "MainWindow.h"
#include "box/BoxWriter.h"
#include "ScriptEditorWindow.h"
#include "ImageInspectorWindow.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QStyle>
#include <QApplication>

namespace
{
// Item-role tag used to tell folders (no payload) apart from file leaves
// (Qt::UserRole holds the index into m_files) - mirrors the ItemType-tag
// pattern AssetTreeWidget already uses for its own tree items.
constexpr int kIsFolderRole = Qt::UserRole + 1;
}

BoxArchiveWindow::BoxArchiveWindow(MainWindow* mainWindow, QWidget* parent)
    : QMainWindow(parent), m_mainWindow(mainWindow)
{
    setWindowTitle("Box Archive");
    resize(900, 650);

    setupUI();
    setupMenus();
    setupToolbar();
    setupStatusBar();
    updateTitle();
}

void BoxArchiveWindow::setupUI()
{
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels({"Name", "Offset", "Size"});
    m_treeWidget->setColumnWidth(0, 500);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &BoxArchiveWindow::onItemDoubleClicked);
    setCentralWidget(m_treeWidget);
}

void BoxArchiveWindow::setupMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* openAction = fileMenu->addAction(tr("&Open Box Archive..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &BoxArchiveWindow::onOpen);

    QAction* newFromFolderAction = fileMenu->addAction(tr("&New Archive from Folder..."));
    connect(newFromFolderAction, &QAction::triggered, this, &BoxArchiveWindow::onNewFromFolder);

    QAction* addFilesAction = fileMenu->addAction(tr("&Add Files..."));
    connect(addFilesAction, &QAction::triggered, this, &BoxArchiveWindow::onAddFiles);

    fileMenu->addSeparator();

    QAction* extractSelAction = fileMenu->addAction(tr("&Extract Selected..."));
    connect(extractSelAction, &QAction::triggered, this, &BoxArchiveWindow::onExtractSelected);

    QAction* extractAllAction = fileMenu->addAction(tr("Extract &All..."));
    connect(extractAllAction, &QAction::triggered, this, &BoxArchiveWindow::onExtractAll);

    fileMenu->addSeparator();

    QAction* saveAction = fileMenu->addAction(tr("&Save Archive"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &BoxArchiveWindow::onSaveArchive);

    QAction* saveAsAction = fileMenu->addAction(tr("Save Archive &As..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &BoxArchiveWindow::onSaveArchiveAs);

    fileMenu->addSeparator();

    QAction* closeAction = fileMenu->addAction(tr("&Close"));
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
}

void BoxArchiveWindow::setupToolbar()
{
    m_toolBar = addToolBar(tr("Box Toolbar"));
    m_toolBar->setMovable(false);

    QAction* openAction = m_toolBar->addAction("Open");
    connect(openAction, &QAction::triggered, this, &BoxArchiveWindow::onOpen);

    QAction* newFromFolderAction = m_toolBar->addAction("New from Folder");
    connect(newFromFolderAction, &QAction::triggered, this, &BoxArchiveWindow::onNewFromFolder);

    QAction* addFilesAction = m_toolBar->addAction("Add Files");
    connect(addFilesAction, &QAction::triggered, this, &BoxArchiveWindow::onAddFiles);

    m_toolBar->addSeparator();

    QAction* extractSelAction = m_toolBar->addAction("Extract Selected");
    connect(extractSelAction, &QAction::triggered, this, &BoxArchiveWindow::onExtractSelected);

    QAction* extractAllAction = m_toolBar->addAction("Extract All");
    connect(extractAllAction, &QAction::triggered, this, &BoxArchiveWindow::onExtractAll);

    m_toolBar->addSeparator();

    QAction* saveAction = m_toolBar->addAction("Save Archive");
    connect(saveAction, &QAction::triggered, this, &BoxArchiveWindow::onSaveArchive);
}

void BoxArchiveWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready - Open a .box archive", this);
    statusBar()->addWidget(m_statusLabel);
}

void BoxArchiveWindow::updateTitle()
{
    QString title = "Box Archive";
    if (!m_currentArchivePath.isEmpty())
    {
        title += " - " + QFileInfo(m_currentArchivePath).fileName();
    }
    setWindowTitle(title);
}

void BoxArchiveWindow::openFile(const QString& filePath)
{
    BoxStruct boxStruct;
    if (!boxStruct.loadBoxFile(filePath))
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to load box archive:\n%1").arg(filePath));
        return;
    }

    m_files = boxStruct.getFiles();
    m_currentArchivePath = filePath;
    m_extractedTempFiles.clear();

    rebuildTree();
    updateTitle();
    m_statusLabel->setText(QString("Loaded %1 files from %2").arg(m_files.size()).arg(QFileInfo(filePath).fileName()));
}

QTreeWidgetItem* BoxArchiveWindow::findOrCreateFolder(QTreeWidgetItem* parent, const QString& name)
{
    // Linear search among direct children only - archive directory depth is
    // small enough (a handful of top-level folders like Units/Images) that
    // this doesn't need a lookup map, mirroring BoxBuilder's own
    // TreeHelper::findOrCreateFolder in spirit.
    QList<QTreeWidgetItem*> children;
    int count = parent ? parent->childCount() : m_treeWidget->topLevelItemCount();
    for (int i = 0; i < count; i++)
    {
        QTreeWidgetItem* child = parent ? parent->child(i) : m_treeWidget->topLevelItem(i);
        if (child->data(0, kIsFolderRole).toBool() && child->text(0) == name)
        {
            return child;
        }
    }

    QTreeWidgetItem* folder = new QTreeWidgetItem(QStringList{name});
    folder->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    folder->setData(0, kIsFolderRole, true);
    if (parent)
    {
        parent->addChild(folder);
    }
    else
    {
        m_treeWidget->addTopLevelItem(folder);
    }
    return folder;
}

void BoxArchiveWindow::rebuildTree()
{
    m_treeWidget->clear();

    for (int i = 0; i < m_files.size(); i++)
    {
        QString normalized = m_files[i].filename;
        normalized.replace('\\', '/');
        QStringList parts = normalized.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty())
        {
            continue;
        }

        QTreeWidgetItem* parent = nullptr;
        for (int p = 0; p < parts.size() - 1; p++)
        {
            parent = findOrCreateFolder(parent, parts[p]);
        }

        QTreeWidgetItem* fileItem = new QTreeWidgetItem(QStringList{
            parts.last(),
            QString::number(m_files[i].offset),
            QString::number(m_files[i].size)
        });
        fileItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        fileItem->setData(0, kIsFolderRole, false);
        fileItem->setData(0, Qt::UserRole, i);

        if (parent)
        {
            parent->addChild(fileItem);
        }
        else
        {
            m_treeWidget->addTopLevelItem(fileItem);
        }
    }

    m_treeWidget->expandAll();
}

void BoxArchiveWindow::extractEntry(int index, const QString& destPath)
{
    if (index < 0 || index >= m_files.size())
    {
        return;
    }

    QFile out(destPath);
    if (!out.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot write to:\n%1").arg(destPath));
        return;
    }
    out.write(m_files[index].data);
    out.close();
}

void BoxArchiveWindow::onOpen()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Box Archive"), QString(), tr("Box Archives (*.box);;All Files (*)"));
    if (!filePath.isEmpty())
    {
        openFile(filePath);
    }
}

bool BoxArchiveWindow::collectFilesFromFolder(const QString& rootDir, QVector<BoxFile>& outFiles)
{
    QDir root(rootDir);
    QDirIterator it(rootDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString absolutePath = it.next();
        QFile f(absolutePath);
        if (!f.open(QIODevice::ReadOnly))
        {
            continue; // unreadable file (permissions, etc.) - skip rather than abort the whole import
        }

        BoxFile bf;
        bf.filename = root.relativeFilePath(absolutePath); // already '/'-separated by QDir
        bf.data = f.readAll();
        bf.size = static_cast<uint32_t>(bf.data.size());
        bf.offset = 0; // recomputed by BoxWriter on save, same as everywhere else in this class
        outFiles.append(bf);
    }

    if (outFiles.isEmpty())
    {
        QMessageBox::information(this, tr("Info"), tr("No files found under:\n%1").arg(rootDir));
        return false;
    }
    return true;
}

void BoxArchiveWindow::onNewFromFolder()
{
    if (!m_files.isEmpty())
    {
        if (QMessageBox::question(this, tr("New Archive from Folder"),
                tr("This discards the currently loaded archive's in-memory contents "
                   "(nothing on disk is touched unless you Save). Continue?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }
    }

    QString folder = QFileDialog::getExistingDirectory(this, tr("Select Folder to Package"));
    if (folder.isEmpty())
    {
        return;
    }

    QVector<BoxFile> collected;
    if (!collectFilesFromFolder(folder, collected))
    {
        return;
    }

    m_files = collected;
    m_currentArchivePath.clear(); // forces Save As on save, same as a brand-new unsaved archive
    m_extractedTempFiles.clear();

    rebuildTree();
    updateTitle();
    m_statusLabel->setText(QString("Staged %1 files from %2 - use Save Archive As... to write the .box file")
        .arg(m_files.size()).arg(folder));
}

void BoxArchiveWindow::onAddFiles()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Add Files to Archive"));
    if (filePaths.isEmpty())
    {
        return;
    }

    bool ok = false;
    QString destFolder = QInputDialog::getText(this, tr("Destination Folder"),
        tr("Folder inside the archive to add these files to (leave empty for the archive root):"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok)
    {
        return;
    }
    destFolder.replace('\\', '/');
    while (destFolder.startsWith('/')) { destFolder.remove(0, 1); }
    while (destFolder.endsWith('/')) { destFolder.chop(1); }

    int added = 0;
    int overwritten = 0;
    int skipped = 0;
    for (const QString& filePath : filePaths)
    {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly))
        {
            skipped++;
            continue;
        }

        QString archiveName = destFolder.isEmpty()
            ? QFileInfo(filePath).fileName()
            : destFolder + "/" + QFileInfo(filePath).fileName();

        int existingIndex = -1;
        for (int i = 0; i < m_files.size(); i++)
        {
            if (m_files[i].filename.compare(archiveName, Qt::CaseInsensitive) == 0)
            {
                existingIndex = i;
                break;
            }
        }

        BoxFile bf;
        bf.filename = archiveName;
        bf.data = f.readAll();
        bf.size = static_cast<uint32_t>(bf.data.size());
        bf.offset = 0;

        if (existingIndex >= 0)
        {
            if (QMessageBox::question(this, tr("File Exists"),
                    tr("\"%1\" already exists in the archive. Overwrite it?").arg(archiveName),
                    QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            {
                skipped++;
                continue;
            }
            m_files[existingIndex] = bf;
            overwritten++;
        }
        else
        {
            m_files.append(bf);
            added++;
        }
    }

    rebuildTree();
    m_statusLabel->setText(QString("Added %1, overwrote %2, skipped %3 - not saved to disk yet")
        .arg(added).arg(overwritten).arg(skipped));
}

void BoxArchiveWindow::onExtractSelected()
{
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (!item || item->data(0, kIsFolderRole).toBool())
    {
        QMessageBox::information(this, tr("Info"), tr("Select a file (not a folder) to extract."));
        return;
    }

    int index = item->data(0, Qt::UserRole).toInt();
    if (index < 0 || index >= m_files.size())
    {
        return;
    }

    QString suggestedName = QFileInfo(m_files[index].filename).fileName();
    QString destPath = QFileDialog::getSaveFileName(this, tr("Extract File"), suggestedName);
    if (destPath.isEmpty())
    {
        return;
    }

    extractEntry(index, destPath);
    m_statusLabel->setText(QString("Extracted: %1").arg(destPath));
}

void BoxArchiveWindow::onExtractAll()
{
    if (m_files.isEmpty())
    {
        QMessageBox::information(this, tr("Info"), tr("No archive loaded."));
        return;
    }

    QString outDir = QFileDialog::getExistingDirectory(this, tr("Extract All To"));
    if (outDir.isEmpty())
    {
        return;
    }

    int extracted = 0;
    for (int i = 0; i < m_files.size(); i++)
    {
        QString normalized = m_files[i].filename;
        normalized.replace('\\', '/');
        QString destPath = QDir(outDir).filePath(normalized);
        QDir().mkpath(QFileInfo(destPath).absolutePath());
        extractEntry(i, destPath);
        extracted++;
    }

    m_statusLabel->setText(QString("Extracted %1 files to %2").arg(extracted).arg(outDir));
    QMessageBox::information(this, tr("Done"), tr("Extracted %1 files.").arg(extracted));
}

void BoxArchiveWindow::onItemDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item || item->data(0, kIsFolderRole).toBool())
    {
        return;
    }

    int index = item->data(0, Qt::UserRole).toInt();
    if (index < 0 || index >= m_files.size())
    {
        return;
    }

    const QString& filename = m_files[index].filename;

    static const QStringList kScriptExtensions = {"cfg", "ai", "oms", "txt"};
    static const QStringList kImageExtensions = {"png", "jpg", "jpeg", "bmp", "gif"};
    QString ext = QFileInfo(filename).suffix().toLower();

    if (!filename.endsWith(".opf", Qt::CaseInsensitive)
        && !kScriptExtensions.contains(ext)
        && !kImageExtensions.contains(ext))
    {
        // No dedicated viewer/editor for this file type - fall back to a
        // plain extract-to-disk, same as "Extract Selected".
        onExtractSelected();
        return;
    }

    // Stable, predictable temp path (not QTemporaryFile - we need the path
    // to survive so the sub-editor's own Save can write back to it, and so
    // we can find it again at archive-rebuild time).
    QString tempPath = QDir::temp().filePath(
        QString("udt_box_%1_%2_%3").arg(QApplication::applicationPid()).arg(index).arg(QFileInfo(filename).fileName()));

    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot create temporary file:\n%1").arg(tempPath));
        return;
    }
    tempFile.write(m_files[index].data);
    tempFile.close();

    m_extractedTempFiles[tempPath] = index;

    if (filename.endsWith(".opf", Qt::CaseInsensitive))
    {
        m_mainWindow->openFile(tempPath);
    }
    else if (kScriptExtensions.contains(ext))
    {
        if (!m_scriptEditor)
        {
            m_scriptEditor = new ScriptEditorWindow(this);
        }
        m_scriptEditor->openFile(tempPath);
        m_scriptEditor->show();
        m_scriptEditor->raise();
        m_scriptEditor->activateWindow();
    }
    else
    {
        if (!m_imageInspector)
        {
            m_imageInspector = new ImageInspectorWindow(this);
        }
        m_imageInspector->openFile(tempPath);
        m_imageInspector->show();
        m_imageInspector->raise();
        m_imageInspector->activateWindow();
    }
}

void BoxArchiveWindow::refreshExtractedEntries()
{
    for (auto it = m_extractedTempFiles.constBegin(); it != m_extractedTempFiles.constEnd(); ++it)
    {
        const QString& tempPath = it.key();
        int index = it.value();
        if (index < 0 || index >= m_files.size() || !QFile::exists(tempPath))
        {
            continue;
        }

        QFile f(tempPath);
        if (f.open(QIODevice::ReadOnly))
        {
            m_files[index].data = f.readAll();
            m_files[index].size = static_cast<uint32_t>(m_files[index].data.size());
            f.close();
        }
    }
}

bool BoxArchiveWindow::saveTo(const QString& filePath)
{
    refreshExtractedEntries();

    BoxWriter writer;
    if (!writer.writeBoxFile(filePath, m_files))
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save archive:\n%1").arg(writer.lastError()));
        return false;
    }
    return true;
}

void BoxArchiveWindow::onSaveArchive()
{
    if (m_files.isEmpty())
    {
        QMessageBox::information(this, tr("Info"), tr("No archive loaded."));
        return;
    }

    if (m_currentArchivePath.isEmpty())
    {
        onSaveArchiveAs();
        return;
    }

    if (QMessageBox::question(this, tr("Save Archive"),
            tr("Overwrite:\n%1\n\nA backup (.bak) of the current file will be kept alongside it.").arg(m_currentArchivePath),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    QString backupPath = m_currentArchivePath + ".bak";
    QFile::remove(backupPath);
    QFile::copy(m_currentArchivePath, backupPath);

    if (saveTo(m_currentArchivePath))
    {
        m_statusLabel->setText(QString("Saved: %1").arg(m_currentArchivePath));
        QMessageBox::information(this, tr("Success"), tr("Archive saved successfully!"));
    }
}

void BoxArchiveWindow::onSaveArchiveAs()
{
    if (m_files.isEmpty())
    {
        QMessageBox::information(this, tr("Info"), tr("No archive loaded."));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Archive As"),
        m_currentArchivePath.isEmpty() ? "Content.box" : m_currentArchivePath, tr("Box Archives (*.box)"));
    if (filePath.isEmpty())
    {
        return;
    }

    if (saveTo(filePath))
    {
        m_currentArchivePath = filePath;
        updateTitle();
        m_statusLabel->setText(QString("Saved: %1").arg(filePath));
        QMessageBox::information(this, tr("Success"), tr("Archive saved successfully!"));
    }
}
