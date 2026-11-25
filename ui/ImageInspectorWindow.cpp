#include "ImageInspectorWindow.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTransform>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

ImageInspectorWindow::ImageInspectorWindow(QWidget* parent)
    : QMainWindow(parent), m_centered(true), m_updatingSizeFields(false)
{
    setWindowTitle("Image Inspector");
    resize(800, 650);

    setupUI();
    setupMenus();
    setupToolbar();
    setupStatusBar();

    m_shortcutManager = new ShortcutManager(this);
    connect(m_shortcutManager, &ShortcutManager::saveRequested, this, &ImageInspectorWindow::onSave);
    connect(m_shortcutManager, &ShortcutManager::centerImageRequested, this, &ImageInspectorWindow::onCenter);
    // Original BoxBuilder naming ("rotateY"/"rotateX") is kept in
    // ShortcutManager (copied verbatim), but what these shortcuts actually
    // do is a horizontal/vertical mirror, not a 3D-style rotation - named
    // accurately here instead.
    connect(m_shortcutManager, &ShortcutManager::rotateYRequested, this, &ImageInspectorWindow::onMirrorHorizontal);
    connect(m_shortcutManager, &ShortcutManager::rotateXRequested, this, &ImageInspectorWindow::onMirrorVertical);

    updateTitle();
}

void ImageInspectorWindow::setupUI()
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    m_imageLabel = new QLabel(central);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(200, 200);
    m_imageLabel->setStyleSheet("background-color: #202020;");
    mainLayout->addWidget(m_imageLabel, 1);

    QWidget* controls = new QWidget(central);
    QHBoxLayout* controlsLayout = new QHBoxLayout(controls);

    controlsLayout->addWidget(new QLabel("Width:", controls));
    m_widthSpin = new QSpinBox(controls);
    m_widthSpin->setRange(1, 8192);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImageInspectorWindow::onSizeFieldChanged);
    controlsLayout->addWidget(m_widthSpin);

    controlsLayout->addWidget(new QLabel("Height:", controls));
    m_heightSpin = new QSpinBox(controls);
    m_heightSpin->setRange(1, 8192);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImageInspectorWindow::onSizeFieldChanged);
    controlsLayout->addWidget(m_heightSpin);

    m_keepAspectCheckbox = new QCheckBox("Keep Aspect Ratio", controls);
    m_keepAspectCheckbox->setChecked(true);
    controlsLayout->addWidget(m_keepAspectCheckbox);

    controlsLayout->addWidget(new QLabel("Format:", controls));
    m_formatCombo = new QComboBox(controls);
    m_formatCombo->addItems({"png", "bmp", "jpg", "jpeg", "gif"});
    controlsLayout->addWidget(m_formatCombo);

    controlsLayout->addStretch(1);

    mainLayout->addWidget(controls);

    setCentralWidget(central);
}

void ImageInspectorWindow::setupMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* saveAction = fileMenu->addAction(tr("&Save"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &ImageInspectorWindow::onSave);

    QAction* saveAsAction = fileMenu->addAction(tr("Save &As..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &ImageInspectorWindow::onSaveAs);

    fileMenu->addSeparator();

    QAction* closeAction = fileMenu->addAction(tr("&Close"));
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &QWidget::close);

    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

    QAction* rotateAction = editMenu->addAction(tr("&Rotate 90°"));
    connect(rotateAction, &QAction::triggered, this, &ImageInspectorWindow::onRotate);

    QAction* mirrorHAction = editMenu->addAction(tr("Mirror &Horizontal (Shift+Y)"));
    connect(mirrorHAction, &QAction::triggered, this, &ImageInspectorWindow::onMirrorHorizontal);

    QAction* mirrorVAction = editMenu->addAction(tr("Mirror &Vertical (Shift+X)"));
    connect(mirrorVAction, &QAction::triggered, this, &ImageInspectorWindow::onMirrorVertical);

    QAction* centerAction = editMenu->addAction(tr("&Center (Shift+C)"));
    connect(centerAction, &QAction::triggered, this, &ImageInspectorWindow::onCenter);
}

void ImageInspectorWindow::setupToolbar()
{
    m_toolBar = addToolBar(tr("Image Toolbar"));
    m_toolBar->setMovable(false);

    QAction* rotateAction = m_toolBar->addAction("Rotate 90°");
    connect(rotateAction, &QAction::triggered, this, &ImageInspectorWindow::onRotate);

    QAction* mirrorHAction = m_toolBar->addAction("Mirror H");
    connect(mirrorHAction, &QAction::triggered, this, &ImageInspectorWindow::onMirrorHorizontal);

    QAction* mirrorVAction = m_toolBar->addAction("Mirror V");
    connect(mirrorVAction, &QAction::triggered, this, &ImageInspectorWindow::onMirrorVertical);

    m_toolBar->addSeparator();

    QAction* saveAction = m_toolBar->addAction("Save");
    connect(saveAction, &QAction::triggered, this, &ImageInspectorWindow::onSave);
}

void ImageInspectorWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready - Open an image", this);
    statusBar()->addWidget(m_statusLabel);
}

void ImageInspectorWindow::updateTitle()
{
    QString title = "Image Inspector";
    if (!m_currentFilePath.isEmpty())
    {
        title += " - " + QFileInfo(m_currentFilePath).fileName();
    }
    setWindowTitle(title);
}

void ImageInspectorWindow::openFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot open:\n%1").arg(filePath));
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QImage img;
    if (!img.loadFromData(data))
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot decode image:\n%1").arg(filePath));
        return;
    }

    m_image = img;
    m_currentFilePath = filePath;

    QString ext = QFileInfo(filePath).suffix().toLower();
    int idx = m_formatCombo->findText(ext);
    if (idx >= 0)
    {
        m_formatCombo->setCurrentIndex(idx);
    }

    updateSizeFieldsFromImage();
    renderImage();
    updateTitle();
    m_statusLabel->setText(QString("Loaded: %1 (%2x%3)").arg(filePath).arg(m_image.width()).arg(m_image.height()));
}

void ImageInspectorWindow::updateSizeFieldsFromImage()
{
    m_updatingSizeFields = true;
    m_widthSpin->setValue(m_image.width());
    m_heightSpin->setValue(m_image.height());
    m_updatingSizeFields = false;
}

void ImageInspectorWindow::renderImage()
{
    if (m_image.isNull())
    {
        m_imageLabel->clear();
        return;
    }

    QImage scaled = m_image.scaled(m_widthSpin->value(), m_heightSpin->value(),
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    m_imageLabel->setAlignment(m_centered ? Qt::AlignCenter : (Qt::AlignLeft | Qt::AlignTop));
    m_imageLabel->setPixmap(QPixmap::fromImage(scaled));
}

void ImageInspectorWindow::onSizeFieldChanged()
{
    if (m_updatingSizeFields || m_image.isNull())
    {
        return;
    }

    if (m_keepAspectCheckbox->isChecked() && m_image.width() > 0 && m_image.height() > 0)
    {
        m_updatingSizeFields = true;
        // Whichever field the user is actively changing drives the other -
        // approximated here by always deriving height from width against
        // the image's original aspect ratio (simple, avoids a feedback
        // loop between two mutually-recomputing spin boxes).
        double aspect = static_cast<double>(m_image.height()) / static_cast<double>(m_image.width());
        m_heightSpin->setValue(qRound(m_widthSpin->value() * aspect));
        m_updatingSizeFields = false;
    }

    renderImage();
}

void ImageInspectorWindow::onRotate()
{
    if (m_image.isNull())
    {
        return;
    }
    QTransform t;
    t.rotate(90);
    m_image = m_image.transformed(t, Qt::SmoothTransformation);
    updateSizeFieldsFromImage();
    renderImage();
}

void ImageInspectorWindow::onMirrorHorizontal()
{
    if (m_image.isNull())
    {
        return;
    }
    m_image = m_image.flipped(Qt::Horizontal);
    renderImage();
}

void ImageInspectorWindow::onMirrorVertical()
{
    if (m_image.isNull())
    {
        return;
    }
    m_image = m_image.flipped(Qt::Vertical);
    renderImage();
}

void ImageInspectorWindow::onCenter()
{
    m_centered = !m_centered;
    renderImage();
}

bool ImageInspectorWindow::saveTo(const QString& filePath)
{
    if (m_image.isNull())
    {
        QMessageBox::information(this, tr("Info"), tr("No image loaded."));
        return false;
    }

    QImage output = m_image.scaled(m_widthSpin->value(), m_heightSpin->value(),
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    if (!output.save(filePath, m_formatCombo->currentText().toUtf8().constData()))
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save:\n%1").arg(filePath));
        return false;
    }
    return true;
}

void ImageInspectorWindow::onSave()
{
    if (m_currentFilePath.isEmpty())
    {
        onSaveAs();
        return;
    }

    if (saveTo(m_currentFilePath))
    {
        m_statusLabel->setText(QString("Saved: %1").arg(m_currentFilePath));
    }
}

void ImageInspectorWindow::onSaveAs()
{
    QString suggested = m_currentFilePath.isEmpty()
        ? QDir::homePath() + "/image." + m_formatCombo->currentText()
        : m_currentFilePath;

    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Image As"), suggested,
        tr("Images (*.png *.bmp *.jpg *.jpeg *.gif)"));
    if (filePath.isEmpty())
    {
        return;
    }

    if (saveTo(filePath))
    {
        m_currentFilePath = filePath;
        updateTitle();
        m_statusLabel->setText(QString("Saved: %1").arg(filePath));
    }
}
