#include "dtoolbar.h"
#include "dicontextbutton.h"
#include "dcheckablebutton.h"
#include "dsearchbar.h"
#include "dcrumbwidget.h"
#include "historystack.h"
#include "dhoverbutton.h"
#include "historystack.h"
#include "dhoverbutton.h"
#include "windowmanager.h"
#include "dfileservices.h"

#include "dfmevent.h"
#include "app/define.h"
#include "app/filesignalmanager.h"

#include "dfilemenumanager.h"

#include "widgets/singleton.h"
#include "views/dfileview.h"
#include "views/dfilemanagerwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>

const int DToolBar::ButtonWidth = 26;
const int DToolBar::ButtonHeight = 24;

DToolBar::DToolBar(QWidget *parent) : QFrame(parent)
{
    initData();
    initUI();
    initConnect();
}

DToolBar::~DToolBar()
{
    delete m_navStack;
}

void DToolBar::initData()
{
//    m_navStack = new HistoryStack(65536);
}

void DToolBar::initUI()
{
    setFocusPolicy(Qt::NoFocus);
    initAddressToolBar();
    initContollerToolBar();

    m_settingsButton = new QPushButton(this);
    m_settingsButton->setFixedWidth(ButtonWidth);
    m_settingsButton->setFixedHeight(ButtonHeight);
    m_settingsButton->setObjectName("settingsButton");
    m_settingsButton->setCheckable(true);
    m_settingsButton->setFocusPolicy(Qt::NoFocus);

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_addressToolBar);
    mainLayout->addWidget(m_contollerToolBar);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_settingsButton);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(14, 0, 14, 0);
    setLayout(mainLayout);
}

void DToolBar::initAddressToolBar()
{
    m_addressToolBar = new QFrame;
    m_addressToolBar->setObjectName("AddressToolBar");
    m_addressToolBar->setFixedHeight(40);

    QHBoxLayout * backForwardLayout = new QHBoxLayout;


    m_backButton = new QPushButton(this);
    m_backButton->setObjectName("backButton");
    m_backButton->setFixedWidth(ButtonWidth);
    m_backButton->setFixedHeight(ButtonHeight);
    m_backButton->setDisabled(true);
    m_backButton->setFocusPolicy(Qt::NoFocus);
    m_forwardButton = new QPushButton(this);
    m_forwardButton->setObjectName("forwardButton");
    m_forwardButton->setFixedWidth(ButtonWidth);
    m_forwardButton->setFixedHeight(ButtonHeight);
    m_forwardButton->setDisabled(true);
    m_forwardButton->setFocusPolicy(Qt::NoFocus);

    m_searchButton = new QPushButton(this);
    m_searchButton->setObjectName("searchButton");
    m_searchButton->setFixedWidth(ButtonWidth);
    m_searchButton->setFixedHeight(ButtonHeight);
    m_searchButton->setFocusPolicy(Qt::NoFocus);

    backForwardLayout->addWidget(m_backButton);
    backForwardLayout->addWidget(m_forwardButton);
    backForwardLayout->setSpacing(0);
    backForwardLayout->setContentsMargins(0, 0, 0, 0);


    QFrame * crumbAndSearch = new QFrame;
    m_searchBar = new DSearchBar(this);
    m_searchBar->hide();
    m_searchBar->setAlignment(Qt::AlignHCenter);
    m_crumbWidget = new DCrumbWidget(this);
    crumbAndSearch->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    QHBoxLayout * comboLayout = new QHBoxLayout;
    comboLayout->addWidget(m_crumbWidget);
    comboLayout->addWidget(m_searchBar);
    comboLayout->addWidget(m_searchButton);
    comboLayout->setSpacing(10);
    comboLayout->setContentsMargins(0, 0, 0, 0);

    crumbAndSearch->setLayout(comboLayout);

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addLayout(backForwardLayout);
    mainLayout->addWidget(crumbAndSearch);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);
    m_addressToolBar->setLayout(mainLayout);
}

void DToolBar::initContollerToolBar()
{
    m_contollerToolBar = new QFrame;
    m_contollerToolBar->setObjectName("ContollerToolBar");
    m_contollerToolBar->setFixedHeight(40);
    m_iconViewButton = new QPushButton(this);
    m_iconViewButton->setFixedWidth(ButtonWidth);
    m_iconViewButton->setFixedHeight(ButtonHeight);
    m_iconViewButton->setObjectName("iconViewButton");
    m_iconViewButton->setCheckable(true);
    m_iconViewButton->setChecked(true);
    m_iconViewButton->setFocusPolicy(Qt::NoFocus);

    m_listViewButton = new QPushButton(this);
    m_listViewButton->setFixedWidth(ButtonWidth);
    m_listViewButton->setFixedHeight(ButtonHeight);
    m_listViewButton->setObjectName("listViewButton");
    m_listViewButton->setCheckable(true);
    m_listViewButton->setFocusPolicy(Qt::NoFocus);

//    m_extendButton = new QPushButton(this);
//    m_extendButton->setFixedHeight(ButtonHeight);
//    m_extendButton->setObjectName("hierarchicalButton");
//    m_extendButton->setCheckable(true);
//    m_extendButton->setFocusPolicy(Qt::NoFocus);

    m_viewButtonGroup = new QButtonGroup(this);
    m_viewButtonGroup->addButton(m_iconViewButton, 0);
    m_viewButtonGroup->addButton(m_listViewButton, 1);
//    m_viewButtonGroup->addButton(m_extendButton, 2);

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_iconViewButton);
    mainLayout->addWidget(m_listViewButton);
//    mainLayout->addWidget(m_extendButton);
    mainLayout->setContentsMargins(10, 0, 0, 0);
    mainLayout->setSpacing(0);
    m_contollerToolBar->setLayout(mainLayout);
}

void DToolBar::initConnect()
{
    connect(m_iconViewButton, &DStateButton::clicked, this, &DToolBar::requestIconView);
    connect(m_listViewButton, &DStateButton::clicked, this, &DToolBar::requestListView);
//    connect(m_extendButton, &DStateButton::clicked, this, &DToolBar::requestExtendView);
    connect(m_backButton, &DStateButton::clicked,this, &DToolBar::backButtonClicked);
    connect(m_forwardButton, &DStateButton::clicked,this, &DToolBar::forwardButtonClicked);
    connect(m_searchBar, &DSearchBar::returnPressed, this, &DToolBar::searchBarTextEntered);
    connect(m_crumbWidget, &DCrumbWidget::crumbSelected, this, &DToolBar::crumbSelected);
    connect(m_crumbWidget, &DCrumbWidget::searchBarActivated, this, &DToolBar::searchBarActivated);
    connect(m_searchButton, &DStateButton::clicked, this, &DToolBar::searchBarClicked);
    connect(m_searchBar, &DSearchBar::focusedOut, this,  &DToolBar::searchBarDeactivated);
    connect(fileSignalManager, &FileSignalManager::currentUrlChanged, this, &DToolBar::crumbChanged);
    connect(fileSignalManager, &FileSignalManager::requestBack, this, &DToolBar::handleHotkeyBack);
    connect(fileSignalManager, &FileSignalManager::requestForward, this, &DToolBar::handleHotkeyForward);
    connect(fileSignalManager, &FileSignalManager::requestSearchCtrlF, this, &DToolBar::handleHotkeyCtrlF);
    connect(fileSignalManager, &FileSignalManager::requestSearchCtrlL, this, &DToolBar::handleHotkeyCtrlL);
    connect(fileSignalManager, &FileSignalManager::requestChangeIconViewMode, this, &DToolBar::handleChangeIconMode);
    connect(fileSignalManager, &FileSignalManager::requestChangeListViewMode, this, &DToolBar::handleChangeListMode);
    connect(fileSignalManager, &FileSignalManager::requestChangeExtendViewMode, this, &DToolBar::handleChangeExtendMode);
}

DSearchBar *DToolBar::getSearchBar()
{
    return m_searchBar;
}

DCrumbWidget *DToolBar::getCrumWidget()
{
    return m_crumbWidget;
}

QPushButton *DToolBar::getSettingsButton()
{
    return m_settingsButton;
}

void DToolBar::searchBarClicked()
{
    searchBarActivated();
}

void DToolBar::searchBarActivated()
{
    m_searchBar->setPlaceholderText(tr("Search or enter address"));
    m_searchBar->show();
    m_crumbWidget->hide();
    m_searchBar->setAlignment(Qt::AlignLeft);
    m_searchBar->clear();
    m_searchBar->setActive(true);
    m_searchBar->setFocus();
    m_searchBar->setCurrentPath(m_crumbWidget->getCurrentUrl());
    m_searchButton->hide();
}

void DToolBar::searchBarDeactivated()
{
    int winId = WindowManager::getWindowId(this);
    DFileManagerWindow* window = qobject_cast<DFileManagerWindow*>(WindowManager::getWindowById(winId));
    if (window){
        if (window->currentUrl().isSearchFile()){

        }
        else{
            m_searchBar->setPlaceholderText("");
            m_searchBar->hide();
            m_crumbWidget->show();
            m_searchBar->clear();
            m_searchBar->setAlignment(Qt::AlignHCenter);
            m_searchBar->setActive(false);
            m_searchBar->window()->setFocus();
            m_searchButton->show();
        }
    }

    DFMEvent event(this);

    emit fileSignalManager->requestFoucsOnFileView(event);
}

/**
 * @brief DToolBar::searchBarTextEntered
 *
 * Set the tab bar when return press is detected
 * on search bar.
 */
void DToolBar::searchBarTextEntered()
{
    QString text = m_searchBar->text();

    if (text.isEmpty()) {
        m_searchBar->clearText();
        return;
    }

    const QString &currentDir = QDir::currentPath();
    const DUrl &currentUrl = m_crumbWidget->getCurrentUrl();

    if (currentUrl.isLocalFile())
        QDir::setCurrent(currentUrl.toLocalFile());

    DUrl inputUrl = DUrl::fromUserInput(text, false);

    QDir::setCurrent(currentDir);

    DFMEvent event(this);

    event.setData(inputUrl);

    emit fileSignalManager->requestChangeCurrentUrl(event);
}

void DToolBar::crumbSelected(const DFMEvent &e)
{
    if(e.eventId() != WindowManager::getWindowId(this))
        return;

    DFMEvent event(this);

    event.setData(e);

    emit fileSignalManager->requestChangeCurrentUrl(event);
}

void DToolBar::crumbChanged(const DFMEvent &event)
{
    if(event.eventId() != WindowManager::getWindowId(this))
        return;

    if (event.sender() && event.sender()->inherits("DCrumbWidget"))
    {
        checkNavHistory(event.fileUrl());
        return;
    }

    if (event.fileUrl().isSearchFile()){
        m_searchBar->show();
        m_crumbWidget->hide();
        m_searchBar->setAlignment(Qt::AlignLeft);
        m_searchBar->clear();
        m_searchBar->setActive(true);
        m_searchBar->setFocus();
        m_searchBar->setText(event.fileUrl().searchKeyword());
        m_searchBar->getPopupList()->hide();
    }else{
        m_searchBar->hide();
        m_crumbWidget->show();
        setCrumb(event.fileUrl());
    }

    if (event.sender() == this)
        return;

    checkNavHistory(event.fileUrl());
}

/**
 * @brief DToolBar::upButtonClicked
 *
 * Move or shrink to the given index of the tabs depending
 * on the amount of tabs shown. This will be triggered when
 * up button is clicked.
 */

void DToolBar::searchBarChanged(QString path)
{
    m_searchBar->setText(path);
}

void DToolBar::backButtonClicked()
{
    DUrl url = m_navStack->back();

    if(!url.isEmpty())
    {
        DFMEvent event(this);
        event.setData(url);
        updateBackForwardButtonsState();
        emit fileSignalManager->requestChangeCurrentUrl(event);
    }
}

void DToolBar::forwardButtonClicked()
{
    DUrl url = m_navStack->forward();
    qDebug() << url << *m_navStack;
    if(!url.isEmpty())
    {
        DFMEvent event(this);
        event.setData(url);
        updateBackForwardButtonsState();
        emit fileSignalManager->requestChangeCurrentUrl(event);
    }
}

void DToolBar::checkViewModeButton(DFileView::ViewMode mode)
{
    switch (mode) {
    case DFileView::IconMode:
        m_iconViewButton->setChecked(true);
        break;
    case DFileView::ListMode:
        m_listViewButton->setChecked(true);
        break;
    case DFileView::ExtendMode:
//        m_extendButton->setChecked(true);
        break;
    default:
        break;
    }
}

void DToolBar::handleHotkeyBack(const DFMEvent &event)
{
    if (event.eventId() == WindowManager::getWindowId(this)) {
        backButtonClicked();
    }
}

void DToolBar::handleHotkeyForward(const DFMEvent &event)
{
    if (event.eventId() == WindowManager::getWindowId(this)) {
        forwardButtonClicked();
    }
}

void DToolBar::handleHotkeyCtrlF(const DFMEvent &event)
{
    if (event.eventId() == WindowManager::getWindowId(this)) {
        searchBarActivated();
        m_searchBar->setText("");
    }
}

void DToolBar::handleHotkeyCtrlL(const DFMEvent &event)
{
    if (event.eventId() == WindowManager::getWindowId(this)) {
        searchBarActivated();
    }
}

void DToolBar::handleChangeIconMode(const DFMEvent &event)
{
    if (event.eventId() == WindowManager::getWindowId(this)) {
        m_iconViewButton->click();
    }
}

void DToolBar::handleChangeListMode(const DFMEvent &event)
{
    if (event.eventId() == WindowManager::getWindowId(this)) {
        m_listViewButton->click();
    }
}

void DToolBar::handleChangeExtendMode(const DFMEvent &event)
{
    if (event.eventId() == WindowManager::getWindowId(this)) {
        m_extendButton->click();
    }
}

void DToolBar::setViewModeButtonVisible(bool isVisible)
{
    m_iconViewButton->setVisible(isVisible);
    m_listViewButton->setVisible(isVisible);
}

void DToolBar::checkNavHistory(DUrl url)
{

    if (!m_navStack)
        return;

    m_navStack->append(url);
    updateBackForwardButtonsState();
}

void DToolBar::addHistoryStack(){
    m_navStacks.append(new HistoryStack(65536));
}

void DToolBar::switchHistoryStack(const int index , const DUrl &url){
    m_navStack = m_navStacks.at(index);
    if(!m_navStack)
        return;
    updateBackForwardButtonsState();
    setCrumb(url);
}

void DToolBar::removeNavStackAt(int index){
    m_navStacks.removeAt(index);

    if(index < m_navStacks.count())
        m_navStack = m_navStacks.at(index);
    else
        m_navStack = m_navStacks.at(m_navStacks.count()-1);

    if(!m_navStack)
        return;
    if(m_navStack->size() > 1)
        m_backButton->setEnabled(true);
    else
        m_backButton->setEnabled(false);

    if(m_navStack->isLast())
        m_forwardButton->setEnabled(false);
    else
        m_forwardButton->setEnabled(true);
}

void DToolBar::moveNavStacks(int from, int to){
    m_navStacks.move(from,to);
}

int DToolBar::navStackCount() const{
    return m_navStacks.count();
}

void DToolBar::setCrumb(const DUrl &url)
{
    m_crumbWidget->setCrumb(url);
}

void DToolBar::updateBackForwardButtonsState()
{
    if(m_navStack->size() <= 1){
        m_backButton->setEnabled(false);
        m_forwardButton->setEnabled(false);
    }
    else{
        if(m_navStack->isFirst())
            m_backButton->setEnabled(false);
        else
            m_backButton->setEnabled(true);

        if(m_navStack->isLast())
            m_forwardButton->setEnabled(false);
        else
            m_forwardButton->setEnabled(true);
    }
}

void DToolBar::setListModeButtonEnabled(const bool &flag)
{
    m_listViewButton->setEnabled(flag);
}

void DToolBar::setIconModeButtonEnabled(const bool &flag)
{
    m_iconViewButton->setEnabled(flag);
}
