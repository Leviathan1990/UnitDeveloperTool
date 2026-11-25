#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QObject>
#include <QMainWindow>

class ShortcutManager : public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManager(QMainWindow *parent = nullptr);

signals:
    void saveRequested();
    void centerImageRequested();
    void rotateYRequested();
    void rotateXRequested();

private:
    QMainWindow *m_mainWindow;
};

#endif // SHORTCUTMANAGER_H
