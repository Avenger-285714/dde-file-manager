#include "dfileview.h"
#include "dfilesystemmodel.h"
#include "fileitem.h"
#include "filemenumanager.h"
#include "dfileitemdelegate.h"
#include "fileinfo.h"
#include "dfilemenu.h"
#include "dscrollbar.h"
#include "windowmanager.h"

#include "../app/global.h"
#include "../app/fmevent.h"
#include "../app/filemanagerapp.h"
#include "../app/filesignalmanager.h"

#include "../controllers/appcontroller.h"
#include "../controllers/filecontroller.h"
#include "../controllers/fileservices.h"

#include <dthememanager.h>

#include <QWheelEvent>
#include <QLineEdit>
#include <QTextEdit>

DWIDGET_USE_NAMESPACE

DFileView::DFileView(QWidget *parent) : DListView(parent)
{
    D_THEME_INIT_WIDGET(DFileView);

    initUI();
    initDelegate();
    initModel();
    initConnects();
    initActions();
}

DFileView::~DFileView()
{

}

void DFileView::initUI()
{
    m_iconSizes << 48 << 64 << 96 << 128 << 256;

    setSpacing(5);
    setResizeMode(QListView::Adjust);
    setOrientation(QListView::LeftToRight, true);
    setIconSize(currentIconSize());
    setTextElideMode(Qt::ElideMiddle);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionRectVisible(true);
    setEditTriggers(QListView::EditKeyPressed | QListView::SelectedClicked);

    setVerticalScrollBar(new DScrollBar);
}

void DFileView::initDelegate()
{
    setItemDelegate(new DFileItemDelegate(this));
}

void DFileView::initModel()
{
    setModel(new DFileSystemModel(this));
}

void DFileView::initConnects()
{
    connect(this, &DFileView::doubleClicked,
            this, &DFileView::openIndex);
    connect(fileSignalManager, &FileSignalManager::requestChangeCurrentUrl,
            this, &DFileView::cd);
    connect(fileSignalManager, &FileSignalManager::refreshFolder,
            model(), &DFileSystemModel::refresh);
    connect(fileSignalManager, &FileSignalManager::requestRename,
            this, static_cast<void (DFileView::*)(const FMEvent&)>(&DFileView::edit));
    connect(fileSignalManager, &FileSignalManager::requestViewSort,
            this, &DFileView::sort);
    connect(fileSignalManager, &FileSignalManager::requestViewSelectAll,
            this, &DFileView::selectAll);
    connect(fileSignalManager, &FileSignalManager::requestSelectFile,
            this, &DFileView::select);
}

void DFileView::initActions()
{
    QAction *copy_action = new QAction(this);

    copy_action->setAutoRepeat(false);
    copy_action->setShortcut(QKeySequence::Copy);

    connect(copy_action, &QAction::triggered,
            this, [this] {
        fileService->copyFiles(selectedUrls());
    });

    QAction *cut_action = new QAction(this);

    cut_action->setAutoRepeat(false);
    cut_action->setShortcut(QKeySequence::Cut);

    connect(cut_action, &QAction::triggered,
            this, [this] {
        fileService->cutFiles(selectedUrls());
    });

    QAction *paste_action = new QAction(this);

    paste_action->setShortcut(QKeySequence::Paste);

    connect(paste_action, &QAction::triggered,
            this, [this] {
        FMEvent event;

        event = currentUrl();
        event = windowId();

        fileService->pasteFile(event);
    });

    QAction *delete_action = new QAction(this);

    delete_action->setAutoRepeat(false);
    delete_action->setShortcut(QKeySequence::Delete);

    connect(delete_action, &QAction::triggered,
            this, [this] {
        fileService->moveToTrash(selectedUrls());
    });

    addAction(copy_action);
    addAction(cut_action);
    addAction(paste_action);
    addAction(delete_action);
}

DFileSystemModel *DFileView::model() const
{
    return qobject_cast<DFileSystemModel*>(DListView::model());
}

DFileItemDelegate *DFileView::itemDelegate() const
{
    return qobject_cast<DFileItemDelegate*>(DListView::itemDelegate());
}

DUrl DFileView::currentUrl() const
{
    return model()->getUrlByIndex(rootIndex());
}

DUrlList DFileView::selectedUrls() const
{
    DUrlList list;

    for(const QModelIndex &index : selectedIndexes()) {
        list << model()->getUrlByIndex(index);
    }

    return list;
}

bool DFileView::isIconViewMode()
{
    return orientation() == Qt::Vertical && isWrapping();
}

int DFileView::columnWidth(int column) const
{
    return m_headerView ? m_headerView->sectionSize(m_logicalIndexs.value(column)) : 100;
}

void DFileView::setColumnWidth(int column, int width)
{
    if(!m_headerView)
        return;

    m_headerView->resizeSection(m_logicalIndexs.value(column), width);
}

int DFileView::columnCount() const
{
    return m_headerView ? m_headerView->count() : 1;
}

QList<int> DFileView::columnRoleList() const
{
    return m_columnRoles;
}

int DFileView::selectedIndexCount() const
{
    return selectedIndexes().count();
}

int DFileView::windowId() const
{
    if(m_windowId == -1)
        m_windowId = WindowManager::getWindowId(window());

    return m_windowId;
}

void DFileView::startKeyboardSearch(int windowId, const QString &key)
{
    if(windowId != this->windowId())
        return;

    keyboardSearch(key);
}

void DFileView::stopKeyboardSearch(int windowId)
{
    if(windowId != this->windowId())
        return;

    stopSearch();
}

void DFileView::setIconSize(const QSize &size)
{
    DListView::setIconSize(size);

    updateViewportMargins();
}

void DFileView::cd(const FMEvent &event)
{
    if(event.windowId() != windowId())
        return;

    const DUrl &fileUrl = event.fileUrl();

    if(fileUrl.isEmpty())
        return;

    if(setCurrentUrl(fileUrl))
        emit fileSignalManager->currentUrlChanged(event);
}

void DFileView::edit(const FMEvent &event)
{
    if(event.windowId() != windowId())
        return;

    DUrl fileUrl = event.fileUrl();

    if(fileUrl.isEmpty())
        return;

    const QModelIndex &index = model()->index(fileUrl);

    edit(index);
}

void DFileView::select(const FMEvent &event)
{
    if(event.windowId() != windowId()) {
        return;
    }

    const QModelIndex &index = model()->index(event.fileUrl());

    selectionModel()->select(index, QItemSelectionModel::Select);
}

void DFileView::switchToListMode()
{
    if(!isIconViewMode())
        return;

    itemDelegate()->hideAllIIndexWidget();

    if(!m_headerView) {
        m_headerView = new QHeaderView(Qt::Horizontal);
        m_headerView->setModel(model());
        m_headerView->setHighlightSections(true);
        m_headerView->setSectionsClickable(true);
        m_headerView->setSortIndicatorShown(true);
        m_headerView->setSectionResizeMode(QHeaderView::Fixed);
        m_headerView->setSectionResizeMode(0, QHeaderView::Stretch);
        m_headerView->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_headerView->setDefaultSectionSize(100);
        m_headerView->setMinimumSectionSize(200);
        m_headerView->resizeSection(2, 50);

        if(selectionModel()) {
            m_headerView->setSelectionModel(selectionModel());
        }

        connect(m_headerView, &QHeaderView::sectionResized,
                this, static_cast<void (DFileView::*)()>(&DFileView::update));
        connect(m_headerView, &QHeaderView::sortIndicatorChanged,
                model(), &QAbstractItemModel::sort);
    }

    for(int i = 0; i < m_headerView->count(); ++i) {
        m_columnRoles << model()->headerDataToRole(model()->headerData(i, m_headerView->orientation(), Qt::DisplayRole));
        m_logicalIndexs << i;

        if(m_columnRoles.last() == DFileSystemModel::FileNameRole) {
            m_headerView->resizeSection(i, width() - (m_headerView->count() - 1) * m_headerView->defaultSectionSize());
        }

        QSignalBlocker blocker(m_headerView);
        Q_UNUSED(blocker)
        m_headerView->setSortIndicator(i, model()->sortOrder());
    }

    addHeaderWidget(m_headerView);

    setIconSize(QSize(30, 30));
    setOrientation(QListView::TopToBottom, false);
    setSpacing(0);
}

void DFileView::switchToIconMode()
{
    if(isIconViewMode())
        return;

    if(m_headerView) {
        removeHeaderWidget(0);

        m_headerView = Q_NULLPTR;
    }

    m_columnRoles.clear();

    setIconSize(currentIconSize());
    setOrientation(QListView::LeftToRight, true);
    setSpacing(5);
}

void DFileView::sort(int windowId, int role)
{
    if(this->windowId() != windowId)
        return;

    if(isIconViewMode()) {
        model()->setSortRole(role);
        model()->sort();
    } else {
        model()->sort(m_columnRoles.indexOf(role));
    }
}

void DFileView::selectAll(int windowId)
{
    if(windowId != WindowManager::getWindowId(window()))
        return;

    DListView::selectAll();
}

void DFileView::contextMenuEvent(QContextMenuEvent *event)
{
    DFileMenu *menu;
    QModelIndex index;

    if (isEmptyArea(event->pos())){
        index = rootIndex();

        const AbstractFileInfo *info = model()->fileInfo(index);
        const QVector<MenuAction> &actions = info->menuActionList(AbstractFileInfo::SpaceArea);

        menu = FileMenuManager::genereteMenuByKeys(actions, FileMenuManager::getDisableActionList(model()->getUrlByIndex(index)));
        DUrlList urls;
        urls.append(currentUrl());
        menu->setUrls(urls);
        menu->setWindowId(m_windowId);
    } else {
        index = indexAt(event->pos());

        const AbstractFileInfo *info = model()->fileInfo(index);
        const QVector<MenuAction> &actions = info->menuActionList(AbstractFileInfo::Normal);

        menu = FileMenuManager::genereteMenuByKeys(actions, FileMenuManager::getDisableActionList(model()->getUrlByIndex(index)));
        menu->setWindowId(m_windowId);

        DUrlList list = selectedUrls();

        if(list.isEmpty())
            list << model()->getUrlByIndex(index);

        menu->setUrls(list);
    }

    menu->exec();
    menu->deleteLater();

    event->accept();
}

void DFileView::wheelEvent(QWheelEvent *event)
{
    if(isIconViewMode() && m_ctrlIsPressed) {
        if(event->angleDelta().y() > 0) {
            enlargeIcon();
        } else {
            shrinkIcon();
        }

        event->accept();
    } else {
        DListView::wheelEvent(event);
    }
}

void DFileView::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Control) {
        m_ctrlIsPressed = true;
    }

    DListView::keyPressEvent(event);
}

void DFileView::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Control) {
        m_ctrlIsPressed = false;
    }

    DListView::keyReleaseEvent(event);
}

void DFileView::showEvent(QShowEvent *event)
{
    DListView::showEvent(event);

    setFocus();
}

void DFileView::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        setDragEnabled(!isEmptyArea(event->pos()));
    }

    if(!(event->buttons() & Qt::RightButton))
        DListView::mousePressEvent(event);
}

void DFileView::commitData(QWidget *editor)
{
    if(!editor)
        return;

    const AbstractFileInfo *fileInfo = model()->fileInfo(itemDelegate()->editingIndex());

    if(!fileInfo)
        return;

    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);

    if(lineEdit) {
        fileService->renameFile(fileInfo->fileUrl(),
                                DUrl(fileInfo->scheme() + "://" + fileInfo->absolutePath()
                                     + "/" + lineEdit->text()));

        return;
    }

    FileIconItem *item = qobject_cast<FileIconItem*>(editor);

    if(item) {
        fileService->renameFile(fileInfo->fileUrl(),
                                DUrl(fileInfo->scheme() + "://" + fileInfo->absolutePath()
                                     + "/" + item->edit->toPlainText()));
    }
}

void DFileView::focusInEvent(QFocusEvent *event)
{
    DListView::focusInEvent(event);

    itemDelegate()->commitDataAndCloseActiveEditor();
}

void DFileView::resizeEvent(QResizeEvent *event)
{
    updateViewportMargins();

    DListView::resizeEvent(event);
}

bool DFileView::isEmptyArea(const QPoint &pos) const
{
    QModelIndex index = indexAt(pos);

    if(selectionModel()->selectedIndexes().contains(index)) {
        return false;
    } else {
        QStyleOptionViewItem option = viewOptions();

        option.rect = visualRect(index);

        const QList<QRect> &geometry_list = itemDelegate()->paintGeomertyss(option, index);

        for(const QRect &rect : geometry_list) {
            if(rect.contains(pos)) {
                return false;
            }
        }
    }

    return true;
}

QSize DFileView::currentIconSize() const
{
    int size = m_iconSizes.value(m_currentIconSizeIndex);

    return QSize(size, size);
}

void DFileView::enlargeIcon()
{
    if(m_currentIconSizeIndex < m_iconSizes.count() - 1)
        ++m_currentIconSizeIndex;

    setIconSize(currentIconSize());
}

void DFileView::shrinkIcon()
{
    if(m_currentIconSizeIndex > 0)
        --m_currentIconSizeIndex;

    setIconSize(currentIconSize());
}

void DFileView::openIndex(const QModelIndex &index)
{
    if(model()->hasChildren(index)){
        FMEvent event;

        event = model()->getUrlByIndex(index);
        event = FMEvent::FileView;
        event = windowId();

        emit fileSignalManager->requestChangeCurrentUrl(event);
    } else {
        emit fileService->openFile(model()->getUrlByIndex(index));
    }
}

void DFileView::keyboardSearch(const QString &search)
{
    if(search.isEmpty())
        return;

    stopSearch();

    DUrl url;

    url.setScheme(SEARCH_SCHEME);
    url.setPath(currentUrl().path());
    url.setQuery(search);

    setCurrentUrl(url);
}

void DFileView::stopSearch()
{
    DUrl url = currentUrl();

    if(!url.isSearchFile()) {
        return;
    }

    url.setFragment("stop");

    FMEvent event;

    event = url;
    event = FMEvent::FileView;
    event = windowId();

    FileServices::instance()->getChildren(event);
}

bool DFileView::setCurrentUrl(const DUrl &fileUrl)
{
    qDebug() << "cd: current url:" << currentUrl() << "to url:" << fileUrl;

    const DUrl &currentUrl = this->currentUrl();

    if(currentUrl == fileUrl)
        return false;

    stopSearch();

    if(fileUrl.isSearchFile()) {
        if(!fileUrl.fragment().isEmpty()) {
            return false;
        }

        switchToListMode();
    }

    QModelIndex index = model()->index(fileUrl);

    if(!index.isValid())
        index = model()->setRootUrl(fileUrl);

    setRootIndex(index);
    model()->setActiveIndex(index);

    emit currentUrlChanged(fileUrl);

    return true;
}

void DFileView::updateViewportMargins()
{
    QMargins margins = viewportMargins();

    if(isIconViewMode()) {
        int contentWidth = width();

        double itemWidth = iconSize().width() * 1.95 + spacing();
        int itemColumn = contentWidth / itemWidth;
        int gapWidth = (contentWidth - itemWidth * itemColumn) / 2 + 2 * spacing();

        setViewportMargins(gapWidth, 0, 0, 0);
    } else {
        margins.setLeft(10);
        margins.setRight(10);
        setViewportMargins(margins);
    }
}
