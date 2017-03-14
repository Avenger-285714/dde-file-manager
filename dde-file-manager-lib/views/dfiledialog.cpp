#include "dfiledialog.h"
#include "dfilesystemmodel.h"
#include "dfileservices.h"
#include "views/dstatusbar.h"
#include "views/dleftsidebar.h"

#include <DTitlebar>

#include <QEventLoop>
#include <QPointer>
#include <QWhatsThis>
#include <QShowEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QDebug>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformdialoghelper.h>

class DFileDialogPrivate
{
public:
    int result = 0;

    QFileDialog::FileMode fileMode = QFileDialog::AnyFile;
    QFileDialog::AcceptMode acceptMode = QFileDialog::AcceptOpen;
    QFileDialog::Options options;
    QEventLoop *eventLoop = Q_NULLPTR;
    QStringList nameFilters;
};

DFileDialog::DFileDialog(QWidget *parent)
    : DFileManagerWindow(parent)
    , d_ptr(new DFileDialogPrivate())
{
    setWindowFlags(windowFlags() | Qt::Dialog);

    if (titleBar())
        titleBar()->setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowTitleHint);

    setAcceptMode(QFileDialog::AcceptOpen);

    DStatusBar *statusBar = getFileView()->statusBar();

    statusBar->rejectButton()->setText(tr("Cancel"));
    connect(statusBar->acceptButton(), &QPushButton::clicked, this, &DFileDialog::onAcceptButtonClicked);
    connect(statusBar->rejectButton(), &QPushButton::clicked, this, &DFileDialog::onRejectButtonClicked);

    QSet<DFMGlobal::MenuAction> whitelist;

    whitelist << DFMGlobal::NewFolder << DFMGlobal::NewDocument << DFMGlobal::DisplayAs
              << DFMGlobal::SortBy << DFMGlobal::Open << DFMGlobal::Rename << DFMGlobal::Delete
              << DFMGlobal::ListView << DFMGlobal::IconView << DFMGlobal::ExtendView << DFMGlobal::NewWord
              << DFMGlobal::NewExcel << DFMGlobal::NewPowerpoint << DFMGlobal::NewText << DFMGlobal::Name
              << DFMGlobal::Size << DFMGlobal::Type << DFMGlobal::CreatedDate << DFMGlobal::LastModifiedDate
              << DFMGlobal::DeletionDate << DFMGlobal::SourcePath <<DFMGlobal::AbsolutePath;

    getFileView()->setMenuActionWhitelist(whitelist);
    getLeftSideBar()->setDisableUrlSchemes(QList<QString>() << "trash" << "network");
    getLeftSideBar()->setAcceptDrops(false);

    DFileService::FileOperatorTypes fileServiceWhitelist = DFileService::OpenUrl/* | DFileService::OpenFile*/
            | DFileService::RenameFile | DFileService::MoveToTrash | DFileService::NewFolder
            | DFileService::NewDocument | DFileService::CreateFileWatcher;

    getFileView()->setFileOperatorWhitelist(fileServiceWhitelist);
    getFileView()->setDragEnabled(false);
    getFileView()->setDragDropMode(QAbstractItemView::NoDragDrop);
    getFileView()->installEventFilter(this);

    connect(getFileView()->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &DFileDialog::selectionFilesChanged);
    connect(getFileView(), &DFileView::rootUrlChanged,
            this, &DFileDialog::currentUrlChanged);
}

DFileDialog::~DFileDialog()
{

}

void DFileDialog::setDirectory(const QString &directory)
{
    setDirectoryUrl(DUrl::fromLocalFile(directory));
}

void DFileDialog::setDirectory(const QDir &directory)
{
    setDirectoryUrl(DUrl::fromLocalFile(directory.absolutePath()));
}

QDir DFileDialog::directory() const
{
    return QDir(directoryUrl().toLocalFile());
}

void DFileDialog::setDirectoryUrl(const DUrl &directory)
{
    getFileView()->cd(directory);
}

QUrl DFileDialog::directoryUrl() const
{
    return getFileView()->rootUrl();
}

void DFileDialog::selectFile(const QString &filename)
{
    DUrl url = currentUrl();
    QDir dir(url.path());

    url.setPath(dir.absoluteFilePath(filename));

    selectUrl(url);
}

QStringList DFileDialog::selectedFiles() const
{
    QStringList list;

    for (const DUrl &url : selectedUrls())
        list << url.toLocalFile();

    return list;
}

void DFileDialog::selectUrl(const QUrl &url)
{
    getFileView()->select(DUrlList() << url);
}

QList<QUrl> DFileDialog::selectedUrls() const
{
    D_DC(DFileDialog);

    DUrlList list = getFileView()->selectedUrls();

    DUrlList::iterator begin = list.begin();

    while (begin != list.end()) {
        const DAbstractFileInfoPointer &fileInfo = getFileView()->model()->fileInfo(*begin);

        if (fileInfo) {
            DUrl newUrl = fileInfo->toLocalFile();

            if (newUrl.isValid()) {
                *begin = newUrl;
            }
        }

        ++begin;
    }

    if (d->acceptMode == QFileDialog::AcceptSave) {
        const DAbstractFileInfoPointer &fileInfo = getFileView()->model()->fileInfo(list.isEmpty() ? getFileView()->rootUrl() : list.first());
        DUrl fileUrl;

        if (list.isEmpty()) {
            fileUrl = fileInfo->getUrlByChildFileName(getFileView()->statusBar()->lineEdit()->text());
        } else {
            fileUrl = fileInfo->getUrlByNewFileName(getFileView()->statusBar()->lineEdit()->text());
        }

        return QList<QUrl>() << fileUrl;
    }

    if (list.isEmpty() && (d->fileMode == QFileDialog::Directory
                           || d->fileMode == QFileDialog::DirectoryOnly)) {
        list << getFileView()->rootUrl();
    }

    return DUrl::toQUrlList(list);
}

/*
    Strip the filters by removing the details, e.g. (*.*).
*/
QStringList qt_strip_filters(const QStringList &filters)
{
    QStringList strippedFilters;
    QRegExp r(QString::fromLatin1(QPlatformFileDialogHelper::filterRegExp));
    const int numFilters = filters.count();
    strippedFilters.reserve(numFilters);
    for (int i = 0; i < numFilters; ++i) {
        QString filterName;
        int index = r.indexIn(filters[i]);
        if (index >= 0)
            filterName = r.cap(1);
        strippedFilters.append(filterName.simplified());
    }
    return strippedFilters;
}

void DFileDialog::setNameFilters(const QStringList &filters)
{
    D_D(DFileDialog);

    d->nameFilters = filters;

    if (testOption(QFileDialog::HideNameFilterDetails))
        getFileView()->statusBar()->setComBoxItems(qt_strip_filters(filters));
    else
        getFileView()->statusBar()->setComBoxItems(filters);

    if (modelCurrentNameFilter().isEmpty())
        selectNameFilter(filters.isEmpty() ? QString() : filters.first());
}

QStringList DFileDialog::nameFilters() const
{
    D_DC(DFileDialog);

    return d->nameFilters;
}

void DFileDialog::selectNameFilter(const QString &filter)
{
    QString key;

    if (testOption(QFileDialog::HideNameFilterDetails)) {
        key = qt_strip_filters(QStringList(filter)).first();
    } else {
        key = filter;
    }

    int index = getFileView()->statusBar()->comboBox()->findText(key);

    selectNameFilterByIndex(index);
}

QString DFileDialog::modelCurrentNameFilter() const
{
    const QStringList &filters = getFileView()->nameFilters();

    if (filters.isEmpty())
        return QString();

    return filters.first();
}

QString DFileDialog::selectedNameFilter() const
{
    const QComboBox *box = getFileView()->statusBar()->comboBox();

    return box ? box->currentText() : QString();
}

void DFileDialog::selectNameFilterByIndex(int index)
{
    D_D(DFileDialog);

    if (index < 0 || index >= getFileView()->statusBar()->comboBox()->count())
        return;

    getFileView()->statusBar()->comboBox()->setCurrentIndex(index);

    QStringList nameFilters = d->nameFilters;

    if (index == nameFilters.size()) {
        QAbstractItemModel *comboModel = getFileView()->statusBar()->comboBox()->model();
        nameFilters.append(comboModel->index(comboModel->rowCount() - 1, 0).data().toString());
        setNameFilters(nameFilters);
    }

    QString nameFilter = nameFilters.at(index);
    QStringList newNameFilters = QPlatformFileDialogHelper::cleanFilterList(nameFilter);
    if (d->acceptMode == QFileDialog::AcceptSave) {
        QString newNameFilterExtension;
        if (newNameFilters.count() > 0)
            newNameFilterExtension = QFileInfo(newNameFilters.at(0)).suffix();

        QString fileName = getFileView()->statusBar()->lineEdit()->text();
        QMimeDatabase db;

        const QString fileNameExtension = db.suffixForFileName(fileName);
        if (!fileNameExtension.isEmpty() && !newNameFilterExtension.isEmpty()) {
            const int fileNameExtensionLength = fileNameExtension.count();
            fileName.replace(fileName.count() - fileNameExtensionLength,
                             fileNameExtensionLength, newNameFilterExtension);
            getFileView()->clearSelection();
            getFileView()->statusBar()->lineEdit()->setText(fileName);
        }
    }

    getFileView()->setNameFilters(newNameFilters);
}

int DFileDialog::selectedNameFilterIndex() const
{
    const QComboBox *box = getFileView()->statusBar()->comboBox();

    return box ? box->currentIndex() : -1;
}

QDir::Filters DFileDialog::filter() const
{
    return getFileView()->filters();
}

void DFileDialog::setFilter(QDir::Filters filters)
{
    getFileView()->setFilters(filters);
}

void DFileDialog::setViewMode(DFileView::ViewMode mode)
{
    getFileView()->setViewMode(mode);
}

DFileView::ViewMode DFileDialog::viewMode() const
{
    return getFileView()->viewMode();
}

void DFileDialog::setFileMode(QFileDialog::FileMode mode)
{
    D_D(DFileDialog);

    d->fileMode = mode;

    switch (static_cast<int>(mode)) {
    case QFileDialog::ExistingFiles:
        getFileView()->setEnabledSelectionModes(QSet<DFileView::SelectionMode>() << QAbstractItemView::ExtendedSelection);
        break;
    default:
        getFileView()->setEnabledSelectionModes(QSet<DFileView::SelectionMode>() << QAbstractItemView::SingleSelection);
        break;
    }
}

void DFileDialog::setAcceptMode(QFileDialog::AcceptMode mode)
{
    D_D(DFileDialog);

    d->acceptMode = mode;

    if (mode == QFileDialog::AcceptOpen) {
        getFileView()->statusBar()->setMode(DStatusBar::DialogOpen);
        getFileView()->statusBar()->acceptButton()->setText(tr("Open"));
        setFileMode(d->fileMode);

        connect(getFileView()->statusBar()->comboBox(),
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                this, &DFileDialog::selectNameFilter);
        connect(getFileView()->statusBar()->comboBox(),
                static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                this, &DFileDialog::selectedNameFilterChanged);
    } else {
        getFileView()->statusBar()->setMode(DStatusBar::DialogSave);
        getFileView()->statusBar()->acceptButton()->setText(tr("Save"));
        getFileView()->setSelectionMode(QAbstractItemView::SingleSelection);
    }
}

QFileDialog::AcceptMode DFileDialog::acceptMode() const
{
    D_DC(DFileDialog);

    return d->acceptMode;
}

void DFileDialog::setLabelText(QFileDialog::DialogLabel label, const QString &text)
{
    switch (static_cast<int>(label)) {
    case QFileDialog::Accept:
        getFileView()->statusBar()->acceptButton()->setText(text);
        break;
    case QFileDialog::Reject:
        getFileView()->statusBar()->rejectButton()->setText(text);
        break;
    default:
        break;
    }
}

QString DFileDialog::labelText(QFileDialog::DialogLabel label) const
{
    switch (static_cast<int>(label)) {
    case QFileDialog::Accept:
        return getFileView()->statusBar()->acceptButton()->text();
    case QFileDialog::Reject:
        return getFileView()->statusBar()->rejectButton()->text();
    default:
        break;
    }

    return QString();
}

void DFileDialog::setOptions(QFileDialog::Options options)
{
    Q_D(DFileDialog);

    d->options = options;

    getFileView()->model()->setReadOnly(options.testFlag(QFileDialog::ReadOnly));
}

bool DFileDialog::testOption(QFileDialog::Option option) const
{
    Q_D(const DFileDialog);

    return d->options & option;
}

void DFileDialog::setCurrentInputName(const QString &name)
{
    getFileView()->statusBar()->lineEdit()->setText(name);

    QMimeDatabase db;

    const QString &suffix = db.suffixForFileName(name);

    if (!suffix.isEmpty()) {
        getFileView()->statusBar()->lineEdit()->setSelection(0, name.length() - suffix.length() - 1);
    }
}

void DFileDialog::accept()
{
    done(QDialog::Accepted);
}

void DFileDialog::done(int r)
{
    D_D(DFileDialog);

    if (d->eventLoop)
        d->eventLoop->exit(r);

    hide();

    emit finished(r);
    if (r == QDialog::Accepted)
        emit accepted();
    else if (r == QDialog::Rejected)
        emit rejected();
}

int DFileDialog::exec()
{
    D_D(DFileDialog);

    if (d->eventLoop) {
        qWarning("DFileDialog::exec: Recursive call detected");
        return -1;
    }

    bool deleteOnClose = testAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_DeleteOnClose, false);

    bool wasShowModal = testAttribute(Qt::WA_ShowModal);
    setAttribute(Qt::WA_ShowModal, true);

    show();

    QPointer<DFileDialog> guard = this;
    QEventLoop eventLoop;
    d->eventLoop = &eventLoop;
    int res = eventLoop.exec(QEventLoop::DialogExec);
    if (guard.isNull())
        return QDialog::Rejected;
    d->eventLoop = 0;

    setAttribute(Qt::WA_ShowModal, wasShowModal);

    if (deleteOnClose)
        delete this;
    return res;
}

void DFileDialog::open()
{
    show();
}

void DFileDialog::reject()
{
    done(QDialog::Rejected);
}

void DFileDialog::showEvent(QShowEvent *event)
{
    Q_D(DFileDialog);

    if (!event->spontaneous() && !testAttribute(Qt::WA_Moved)) {
        Qt::WindowStates  state = windowState();
        adjustPosition(parentWidget());
        setAttribute(Qt::WA_Moved, false); // not really an explicit position
        if (state != windowState())
            setWindowState(state);
    }
}

void DFileDialog::closeEvent(QCloseEvent *event)
{
#ifndef QT_NO_WHATSTHIS
    if (isModal() && QWhatsThis::inWhatsThisMode())
        QWhatsThis::leaveWhatsThisMode();
#endif
    if (isVisible()) {
        QPointer<QObject> that = this;
        reject();
        if (that && isVisible())
            event->ignore();
    } else {
        event->accept();
    }
}

bool DFileDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == getFileView()
            && (event->type() == QEvent::KeyPress
                || event->type() == QEvent::KeyRelease)) {
        QKeyEvent *e = static_cast<QKeyEvent*>(event);

        if (e->modifiers() == Qt::ControlModifier
                && (e->key() == Qt::Key_T
                    || e->key() == Qt::Key_W)) {
            return true;
        } else if (e->modifiers() == Qt::NoModifier && e->key() == Qt::Key_Escape) {
            close();
        }
    }

    return DFileManagerWindow::eventFilter(watched, event);
}

void DFileDialog::adjustPosition(QWidget *w)
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        if (theme->themeHint(QPlatformTheme::WindowAutoPlacement).toBool())
            return;
    QPoint p(0, 0);
    int extraw = 0, extrah = 0, scrn = 0;
    if (w)
        w = w->window();
    QRect desk;
    if (w) {
        scrn = QApplication::desktop()->screenNumber(w);
    } else if (QApplication::desktop()->isVirtualDesktop()) {
        scrn = QApplication::desktop()->screenNumber(QCursor::pos());
    } else {
        scrn = QApplication::desktop()->screenNumber(this);
    }
    desk = QApplication::desktop()->availableGeometry(scrn);

    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; (extraw == 0 || extrah == 0) && i < list.size(); ++i) {
        QWidget * current = list.at(i);
        if (current->isVisible()) {
            int framew = current->geometry().x() - current->x();
            int frameh = current->geometry().y() - current->y();

            extraw = qMax(extraw, framew);
            extrah = qMax(extrah, frameh);
        }
    }

    // sanity check for decoration frames. With embedding, we
    // might get extraordinary values
    if (extraw == 0 || extrah == 0 || extraw >= 10 || extrah >= 40) {
        extrah = 40;
        extraw = 10;
    }


    if (w && (w->windowFlags() | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint) != w->windowFlags()) {
        // Use pos() if the widget is embedded into a native window
        QPoint pp;
        if (w->windowHandle() && w->windowHandle()->property("_q_embedded_native_parent_handle").value<WId>())
            pp = w->pos();
        else
            pp = w->mapToGlobal(QPoint(0,0));
        p = QPoint(pp.x() + w->width()/2,
                    pp.y() + w->height()/ 2);
    } else {
        // p = middle of the desktop
        p = QPoint(desk.x() + desk.width()/2, desk.y() + desk.height()/2);
    }

    // p = origin of this
    p = QPoint(p.x()-width()/2 - extraw,
                p.y()-height()/2 - extrah);


    if (p.x() + extraw + width() > desk.x() + desk.width())
        p.setX(desk.x() + desk.width() - width() - extraw);
    if (p.x() < desk.x())
        p.setX(desk.x());

    if (p.y() + extrah + height() > desk.y() + desk.height())
        p.setY(desk.y() + desk.height() - height() - extrah);
    if (p.y() < desk.y())
        p.setY(desk.y());

    move(p);
}

void DFileDialog::onAcceptButtonClicked()
{
    D_DC(DFileDialog);

    if (d->acceptMode == QFileDialog::AcceptSave) {
        if (!getFileView()->statusBar()->lineEdit()->text().isEmpty())
            accept();
        return;
    }

    const DUrlList &urls = getFileView()->selectedUrls();

    switch (static_cast<int>(d->fileMode)) {
    case QFileDialog::AnyFile:
    case QFileDialog::ExistingFile:
        if (urls.count() == 1) {
            const DAbstractFileInfoPointer &fileInfo = getFileView()->model()->fileInfo(urls.first());

            if (fileInfo->isDir())
                getFileView()->cd(urls.first());
            else
                accept();
        }
        break;
    case QFileDialog::ExistingFiles:
        for (const DUrl &url : urls) {
            const DAbstractFileInfoPointer &fileInfo = getFileView()->model()->fileInfo(url);

            if (!fileInfo->isFile())
                return;
        }

        if (!urls.isEmpty())
            accept();
        break;
    default:
        for (const DUrl &url : urls) {
            const DAbstractFileInfoPointer &fileInfo = getFileView()->model()->fileInfo(url);

            if (!fileInfo->isDir())
                return;
        }
        accept();
        break;
    }
}

void DFileDialog::onRejectButtonClicked()
{
    reject();
}
