/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     Gary Wang <wzc782970009@gmail.com>
 *
 * Maintainer: Gary Wang <wangzichong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QHBoxLayout>
#include <QPainter>
#include <QScrollBar>
#include <QApplication>
#include <QMenu>
#include <QDebug>
#include <QGuiApplication>
#include <QClipboard>
#include <dgiosettings.h>

#include "dfmapplication.h"
#include "dfmcrumbbar.h"
#include "dfmcrumbitem.h"
#include "dfmcrumbmanager.h"
#include "dfmsettings.h"
#include "views/windowmanager.h"
#include "views/dfilemanagerwindow.h"
#include "views/dfmaddressbar.h"
#include "views/themeconfig.h"
#include "dfmcrumbfactory.h"
#include "dfmcrumbinterface.h"
#include "dfmapplication.h"
#include "dfmevent.h"
#include "dlistview.h"
#include "dfmcrumblistviewmodel.h"
#include "app/define.h"
#include "singleton.h"
#include "interfaces/dfmstandardpaths.h"
#include "controllers/pathmanager.h"

DWIDGET_USE_NAMESPACE

DFM_BEGIN_NAMESPACE

class DFMCrumbBarPrivate
{
    Q_DECLARE_PUBLIC(DFMCrumbBar)

public:
    DFMCrumbBarPrivate(DFMCrumbBar *qq);

    // UI
    QPushButton leftArrow;
    QPushButton rightArrow;
    DListView crumbListView;
    DFMCrumbListviewModel *crumbListviewModel = nullptr;
    QHBoxLayout *crumbBarLayout;
    QPoint clickedPos;
    DFMAddressBar *addressBar = nullptr;

    // Scheme support
    DFMCrumbInterface* crumbController = nullptr;

    // Misc
    bool clickableAreaEnabled = false;

    DFMCrumbBar *q_ptr = nullptr;

    void clearCrumbs();
    void checkArrowVisiable();
    void updateController(const DUrl &url);
    void setClickableAreaEnabled(bool enabled);

private:
    void initUI();
    void initData();
    void initConnections();
};

DFMCrumbBarPrivate::DFMCrumbBarPrivate(DFMCrumbBar *qq)
    : q_ptr(qq)
{
    initData();
    initUI();
    initConnections();
}

/*!
 * \brief Remove all crumbs List items inside crumb listView.
 */
void DFMCrumbBarPrivate::clearCrumbs()
{
    leftArrow.hide();
    rightArrow.hide();

    if (crumbListviewModel)
        crumbListviewModel->removeAll();
}

void DFMCrumbBarPrivate::checkArrowVisiable()
{
    QScrollBar* sb = crumbListView.horizontalScrollBar();
    if (!sb)
        return;

    leftArrow.setVisible(!addressBar->isVisible() && sb->maximum() > 0);
    rightArrow.setVisible(!addressBar->isVisible() && sb->maximum() > 0);

    leftArrow.setEnabled(sb->value() != sb->minimum());
    rightArrow.setEnabled(sb->value() != sb->maximum());
}

/*!
 * \brief Update the crumb controller hold by the crumb bar.
 *
 * \param url The url which controller should supported.
 */
void DFMCrumbBarPrivate::updateController(const DUrl &url)
{
    Q_Q(DFMCrumbBar);

    if (!crumbController || !crumbController->supportedUrl(url)) {
        if (crumbController) {
            crumbController->deleteLater();
        }
        crumbController = DFMCrumbManager::instance()->createControllerByUrl(url, q);
        // Not found? Then nothing here...
        if (!crumbController) {
            qDebug() << "Unsupported url / scheme: " << url;
        }
    }
}

void DFMCrumbBarPrivate::setClickableAreaEnabled(bool enabled)
{
    Q_Q(DFMCrumbBar);

    if (clickableAreaEnabled == enabled) return;

    clickableAreaEnabled = enabled;
    //crumbListHolder->setContentsMargins(0, 0, (enabled ? 30 : 0), 0);

    q->update();
}

void DFMCrumbBarPrivate::initUI()
{
    Q_Q(DFMCrumbBar);

    // Address Bar
    addressBar->hide();

    // Crumbbar Widget
    q->setFixedHeight(24);

    // Arrows
    QSize size(24, 24), iconSize(16, 16);
    leftArrow.setFocusPolicy(Qt::NoFocus);
    leftArrow.setIcon(QIcon::fromTheme("go-previous").pixmap(iconSize));
    rightArrow.setIcon(QIcon::fromTheme("go-next").pixmap(iconSize));
    rightArrow.setFocusPolicy(Qt::NoFocus);

    leftArrow.setFixedSize(size);
    leftArrow.setIconSize(iconSize);
    rightArrow.setFixedSize(size);
    rightArrow.setIconSize(iconSize);
    leftArrow.setFlat(true);
    rightArrow.setFlat(true);
    leftArrow.hide();
    rightArrow.hide();

    // Crumb List Layout
    crumbListView.setObjectName("DCrumbListScrollArea");
    crumbListView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    crumbListView.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    crumbListView.setFocusPolicy(Qt::NoFocus);
    crumbListView.setContentsMargins(0, 0, 0, 0);
    crumbListView.setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    crumbListView.setIconSize({16, 16});

    crumbListView.QListView::setFlow(QListView::LeftToRight);
    crumbListView.QListView::setWrapping(false);
    crumbListView.setEditTriggers(QAbstractItemView::NoEditTriggers);
    crumbListView.setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);

    crumbListviewModel = new DFMCrumbListviewModel;
    crumbListView.setModel(crumbListviewModel);
    crumbListView.setContextMenuPolicy(Qt::CustomContextMenu);

    // 点击listview空白可拖动窗口
    crumbListView.viewport()->installEventFilter(q);

    // Crumb Bar Layout
    crumbBarLayout = new QHBoxLayout;
    crumbBarLayout->addWidget(&leftArrow);
    crumbBarLayout->addWidget(&crumbListView);
    crumbBarLayout->addWidget(&rightArrow);
    crumbBarLayout->setContentsMargins(0,0,0,0);
    crumbBarLayout->setSpacing(0);
    q->setLayout(crumbBarLayout);

    return;
}

void DFMCrumbBarPrivate::initData()
{
    Q_Q(DFMCrumbBar);
    addressBar = new DFMAddressBar(q);
    clickableAreaEnabled = DFMApplication::instance()->genericAttribute(DFMApplication::GA_ShowCsdCrumbBarClickableArea).toBool();
}

void DFMCrumbBarPrivate::initConnections()
{
    Q_Q(DFMCrumbBar);
    QObject::connect(&crumbListView, &QListView::customContextMenuRequested, q, &DFMCrumbBar::onListViewContextMenu);

    QObject::connect(&crumbListView, &QListView::clicked, q, [q](const QModelIndex &index){
        if (index.isValid()){
            emit q->crumbListItemSelected(index.data(DFMCrumbListviewModel::FileUrlRole).toUrl());
        }
    });

    q->connect(&leftArrow, &QPushButton::clicked, q, [this]() {
        crumbListView.horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    });

    q->connect(&rightArrow, &QPushButton::clicked, q, [this]() {
        crumbListView.horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    });

    q->connect(crumbListView.horizontalScrollBar(), &QScrollBar::valueChanged, q, [this]() {
        checkArrowVisiable();
    });

    q->connect(addressBar, &DFMAddressBar::returnPressed, q, [q, this]() {
        emit q->addressBarContentEntered(addressBar->text());
    });

    q->connect(addressBar, &DFMAddressBar::escKeyPressed, q, [this]() {
        if (crumbController) {
            crumbController->processAction(DFMCrumbInterface::EscKeyPressed);
        }
    });

    q->connect(addressBar, &DFMAddressBar::lostFocus, q, [q, this]() {
        if (crumbController && !qobject_cast<DFileManagerWindow*>(q->window())->isAdvanceSearchBarVisible()) {
            crumbController->processAction(DFMCrumbInterface::AddressBarLostFocus);
        }
    });

    q->connect(addressBar, &DFMAddressBar::clearButtonPressed, q, [this] {
        if (crumbController) {
            crumbController->processAction(DFMCrumbInterface::ClearButtonPressed);
        }
    });

    q->connect(DFMApplication::instance(), &DFMApplication::csdClickableAreaAttributeChanged, q, [this](bool enabled) {
        setClickableAreaEnabled(enabled);
    });
}

/*!
 * \class DFMCrumbBar
 * \inmodule dde-file-manager-lib
 *
 * \brief DFMCrumbBar is the crumb bar widget of Deepin File Manager
 *
 * DFMCrumbBar is the crumb bar widget of Deepin File Manager, provide the interface to manage
 * crumb bar state. Crumb bar holds a group of DFMCrumbItem s, and crumb bar will be switch to
 * DFMAddressBar when clicking empty space at crumb bar.
 *
 * \sa DFMCrumbInterface, DFMCrumbManager, DFMCrumbItem
 */

DFMCrumbBar::DFMCrumbBar(QWidget *parent)
    : QFrame(parent)
    , d_ptr(new DFMCrumbBarPrivate(this))
{
    setFrameShape(QFrame::NoFrame);
}

DFMCrumbBar::~DFMCrumbBar()
{

}

/*!
 * \brief Toggle and show the address bar.
 *
 * \param text The text in the address bar.
 */
void DFMCrumbBar::showAddressBar(const QString &text)
{
    Q_D(DFMCrumbBar);

    d->leftArrow.hide();
    d->rightArrow.hide();
    d->crumbListView.hide();

    d->addressBar->show();
    d->addressBar->setText(text);
    //d->addressBar->setSelection(0, text.length());
    d->addressBar->setFocus();

    emit addressBarShown();

    return;
}

/*!
 * \brief Toggle and show the address bar.
 *
 * \param url The url in the address bar.
 */
void DFMCrumbBar::showAddressBar(const DUrl &url)
{
    Q_D(DFMCrumbBar);

    d->leftArrow.hide();
    d->rightArrow.hide();
    d->crumbListView.hide();

    d->addressBar->show();
    d->addressBar->setCurrentUrl(url);
    d->addressBar->setFocus();

    emit addressBarShown();

    return;
}

/*!
 * \brief Hide the address bar.
 */
void DFMCrumbBar::hideAddressBar()
{
    Q_D(DFMCrumbBar);

    d->addressBar->hide();

    d->crumbListView.show();
    d->checkArrowVisiable();

    emit addressBarHidden();

    return;
}

static QString getIconName(const CrumbData& c)
{
    QString iconName;
    if (c.url.isComputerFile()) {
        iconName = systemPathManager->getSystemPathIconName("Computer");
    } else if (c.url.isRecentFile()) {
        iconName = systemPathManager->getSystemPathIconName("Recent");
    } else if (c.url.isTrashFile()) {
        iconName = systemPathManager->getSystemPathIconName("Trash");
    } else if (c.url.isNetWorkFile()) {
        iconName = systemPathManager->getSystemPathIconName("Network");
    } else if (c.url.isUserShareFile()) {
        iconName = systemPathManager->getSystemPathIconName("UserShare");
    }

    if (iconName.isEmpty()) {
        QString path = c.url.toLocalFile();
        static QString home = DFMStandardPaths::location(DFMStandardPaths::HomePath);
        if (path.size()>home.size() && path.startsWith(home))
            return QString(); // 主目录下的（视频，文档，图片。。）不显示icon

        iconName = systemPathManager->getSystemPathIconNameByPath(path);
    }

    static QMap<QString, QString> s_mapIconName = {
        {"CrumbIconButton.Disk","drive-harddisk-symbolic"},
        {"CrumbIconButton.Usb","drive-harddisk-usb-symbolic"},
        {"CrumbIconButton.Dvd","media-optical-dvd-symbolic"},
        {"CrumbIconButton.Iphone","iphone-symbolic"},
        {"CrumbIconButton.Android","android-phone-symbolic"},
        {"BookmarkItem.Orange", "dfm_tag-orange"},
        {"BookmarkItem.Red", "dfm_tag-red"},
        {"BookmarkItem.Purple", "dfm_tag-purple"},
        {"BookmarkItem.Navy-blue", "dfm_tag-deepblue"},
        {"BookmarkItem.Azure", "dfm_tag-lightblue"},
        {"BookmarkItem.Grass-green", "dfm_tag-green"},
        {"BookmarkItem.Yellow", "dfm_tag-yellow"},
        {"BookmarkItem.Gray", "dfm_tag-gray"}
    };

    if (iconName.isEmpty() && s_mapIconName.contains(c.iconName)) {
        iconName = s_mapIconName[c.iconName];
    }

    if (!iconName.isEmpty() && !iconName.startsWith("dfm_") && !iconName.contains("-symbolic"))
        iconName.append("-symbolic");

    return iconName;
}

/*!
 * \brief Update crumbs in crumb bar by the given \a url
 *
 * \param url The newly switched url.
 *
 * DFMCrumbBar holds an instance of crumb controller (derived from DFMCrumbInterface), and when
 * calling updateCrumb, it will check the current used crumb controller is supporting the given
 * \a url. if isn't, we will create a new crumb controller form the registed controllers or the
 * plugin list. Then we will call DFMCrumbInterface::seprateUrl to seprate the url and call
 * DFMCrumbInterface::createCrumbItem to create the crumb item. Finally, we added the created
 * items to the crumb bar.
 */
void DFMCrumbBar::updateCrumbs(const DUrl &url)
{
    Q_D(DFMCrumbBar);

    d->clearCrumbs();

    if (!d->crumbController) {
        qWarning("No controller found when trying to call DFMCrumbBar::updateCrumbs() !!!");
        qDebug() << "updateCrumbs (no controller) : " << url;
        return;
    }

    QList<CrumbData> crumbDataList = d->crumbController->seprateUrl(url);
    for (const CrumbData& c : crumbDataList) {
        if (d->crumbListviewModel) {
            QString iconName = getIconName(c);
            QStandardItem *listitem = nullptr;
            if (!iconName.isEmpty()) {
                listitem = new QStandardItem(QIcon::fromTheme(iconName), QString());
            } else {
                listitem = new QStandardItem(c.displayText);
            }

            listitem->setCheckable(false);
            listitem->setData(c.url, DFMCrumbListviewModel::FileUrlRole);
            d->crumbListviewModel->appendRow(listitem);
        }
    }

    if (d->crumbListView.selectionModel() && d->crumbListviewModel)
        d->crumbListView.selectionModel()->select(d->crumbListviewModel->lastIndex(), QItemSelectionModel::Select);

    d->checkArrowVisiable();
    d->crumbListView.horizontalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
}

void DFMCrumbBar::playAddressBarAnimation()
{
    Q_D(DFMCrumbBar);

    d->addressBar->playAnimation();
}

void DFMCrumbBar::stopAddressBarAnimation()
{
    Q_D(DFMCrumbBar);

    d->addressBar->stopAnimation();
}

/*!
 * \brief Call when toolbar url got changed.
 *
 * \param url Current url which we changed to.
 */
void DFMCrumbBar::updateCurrentUrl(const DUrl &url)
{
    Q_D(DFMCrumbBar);

    d->updateController(url);

    if (d->crumbController) {
        d->crumbController->crumbUrlChangedBehavior(url);
    }
}

void DFMCrumbBar::mousePressEvent(QMouseEvent *event)
{
    Q_D(DFMCrumbBar);
    d->clickedPos = event->globalPos();

    if (event->button() == Qt::RightButton && d->clickableAreaEnabled) {
        event->accept();
        return;
    }

    QFrame::mousePressEvent(event);
}

void DFMCrumbBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(DFMCrumbBar);

    if (!d->clickableAreaEnabled) {
        return QFrame::mouseReleaseEvent(event);;
    }

    // blumia: no need to check if it's clicked on other widgets
    //         since this will only happend when clicking empty.
    const QPoint pos_difference = d->clickedPos - event->globalPos();

    if (qAbs(pos_difference.x()) < 2 && qAbs(pos_difference.y()) < 2) {
        showAddressBar(qobject_cast<DFileManagerWindow*>(topLevelWidget())->currentUrl());
    }

    QFrame::mouseReleaseEvent(event);
}

void DFMCrumbBar::resizeEvent(QResizeEvent *event)
{
    Q_D(DFMCrumbBar);

    d->checkArrowVisiable();
    d->addressBar->resize(event->size());

    return QFrame::resizeEvent(event);
}

void DFMCrumbBar::showEvent(QShowEvent *event)
{
    Q_D(DFMCrumbBar);

    //d->crumbListScrollArea.horizontalScrollBar()->setPageStep(d->crumbListHolder->width());
    d->crumbListView.horizontalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);

    d->checkArrowVisiable();

    return QFrame::showEvent(event);
}

bool DFMCrumbBar::eventFilter(QObject *watched, QEvent *event)
{
    Q_D(DFMCrumbBar);
    if (watched && watched->parent() == &d->crumbListView
            && (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonDblClick))
        event->ignore();

    return QFrame::eventFilter(watched, event);
}

void DFMCrumbBar::onListViewContextMenu(const QPoint &point)
{
    Q_D(DFMCrumbBar);
    QModelIndex index = d->crumbListView.indexAt(point);
    if (!index.isValid())
        return ;

    QMenu *menu = new QMenu();
    DUrl url = index.data(DFMCrumbListviewModel::FileUrlRole).toUrl();
    DGioSettings settings("com.deepin.dde.filemanager.general", "/com/deepin/dde/filemanager/general/");
    bool displayIcon = settings.value("context-menu-icons").toBool();
    QIcon copyIcon, newWndIcon,  newTabIcon, editIcon;
    if (displayIcon) {
        copyIcon = QIcon::fromTheme("edit-copy");
        newWndIcon = QIcon::fromTheme("window-new");
        newTabIcon = QIcon::fromTheme("tab-new");
        editIcon = QIcon::fromTheme("entry-edit");
    }

    DFileManagerWindow *wnd = qobject_cast<DFileManagerWindow *>(window());
    bool shouldDisable = !WindowManager::tabAddableByWinId(wnd->windowId());

    menu->addAction(copyIcon, QObject::tr("Copy path"), [=]() {
        QGuiApplication::clipboard()->setText(url.toString());
    });

    menu->addAction(newWndIcon, QObject::tr("Open in new window"), [url]() {
        WindowManager::instance()->showNewWindow(url, true);
    });

    menu->addAction(newTabIcon, QObject::tr("Open in new tab"), [wnd,url]() {
        wnd->openNewTab(url);
    })->setDisabled(shouldDisable);

    menu->addSeparator();

    menu->addAction(editIcon, QObject::tr("Edit address"), this, [=]() {
        showAddressBar(wnd->currentUrl());
    });

    menu->exec(QCursor::pos());
    menu->deleteLater();
}

/*!
 * \fn DFMCrumbBar::addressBarShown()
 *
 * \brief Emit when the address bar got shown.
 *
 * \sa DFMAddressBar
 */

/*!
 * \fn DFMCrumbBar::addressBarHidden()
 *
 * \brief Emit when the address bar got hidden.
 *
 * \sa DFMAddressBar
 */

/*!
 * \fn DFMCrumbBar::crumbItemClicked(DFMCrumbItem *item);
 *
 * \brief User clicked the crumb \a item.
 */

DFM_END_NAMESPACE
