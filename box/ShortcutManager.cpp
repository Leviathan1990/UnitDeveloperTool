#include "ShortcutManager.h"
#include <QShortcut>
#include <QKeySequence>

ShortcutManager::ShortcutManager(QMainWindow *parent) : QObject(parent), m_mainWindow(parent)
{
    if (m_mainWindow)
    {
        //  IMAGE INSPECTOR

        // CTRL + S shortcut    // For save image in Image Inspector
        QShortcut *saveShortcut = new QShortcut(QKeySequence("Ctrl+S"), m_mainWindow);
        connect(saveShortcut, &QShortcut::activated, this, &ShortcutManager::saveRequested);

        // Shift + C shortcut: Image center
        QShortcut *centerShortcut = new QShortcut(QKeySequence("Shift+C"), m_mainWindow);
        connect(centerShortcut, &QShortcut::activated, this, &ShortcutManager::centerImageRequested);

        // Shift + Y shortcut: Rotate 90 degrees around Y axis (if implemented)
        QShortcut *rotateYShortcut = new QShortcut(QKeySequence("Shift+Y"), m_mainWindow);
        connect(rotateYShortcut, &QShortcut::activated, this, &ShortcutManager::rotateYRequested);

        // Shift + X shortcut: Rotate 90 degrees around X axis (if implemented)
        QShortcut *rotateXShortcut = new QShortcut(QKeySequence("Shift+X"), m_mainWindow);
        connect(rotateXShortcut, &QShortcut::activated, this, &ShortcutManager::rotateXRequested);

    }
}
