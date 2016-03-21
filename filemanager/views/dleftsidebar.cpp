#include "dleftsidebar.h"
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include "../app/global.h"
#include "dcheckablebutton.h"
#include "dhorizseparator.h"
#include <QListWidgetItem>


DLeftSideBar::DLeftSideBar(QWidget *parent) : QFrame(parent)
{
    initData();
    initUI();
    initConnect();
}

DLeftSideBar::~DLeftSideBar()
{

}

void DLeftSideBar::initData()
{
    m_iconlist << ":/icons/images/icons/sidebar_expand_normal.png" //file
               << ":/icons/images/icons/folder-recent-symbolic.svg" //recent
               << ":/icons/images/icons/user-home-symbolic.svg" //home
               << ":/icons/images/icons/folder-desktop-symbolic.svg" //desktop
               << ":/icons/images/icons/folder-videos-symbolic.svg" //video
               << ":/icons/images/icons/folder-pictures-symbolic.svg" //picture
               << ":/icons/images/icons/folder-documents-symbolic.svg" //document
               << ":/icons/images/icons/folder-download-symbolic.svg" //download
               << ":/icons/images/icons/folder-music-symbolic.svg" //music
               << ":/icons/images/icons/user-trash-symbolic.svg" //trash
               << ":/icons/images/icons/drive-removable-media-symbolic.svg" //disk
               << ":/images/images/dark/appbar.iphone.png" //my mobile
               << ":/icons/images/icons/user-bookmarks-symbolic.svg";//bookmarks

    m_nameList << "File" << "Recent" << "Home"  << "Desktop"
               << "Videos" << "Pictures" << "Documents" << "Downloads" << "Musics"
               << "Trash" << "Disks" << "My Mobile" << "Bookmarks" ;
    m_navState = true;
}

void DLeftSideBar::initUI()
{
    initTightNav();
    initNav();
    m_stackedWidget = new QStackedWidget;
    m_stackedWidget->addWidget(m_tightNav);
    m_stackedWidget->addWidget(m_nav);


    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_stackedWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout);
    m_stackedWidget->setCurrentIndex(1);
}

//void DLeftSideBar::initHomeBar()
//{
//    m_homeButton = new DCheckableButton(":/icons/images/icons/user-home-symbolic.svg", tr("Home"), this);
//    m_recentButton = new DCheckableButton(":/icons/images/icons/folder-recent-symbolic.svg", tr("Recent"), this);
//    m_buttonGroup->addButton(m_recentButton, 1);
//    m_buttonGroup->addButton(m_homeButton, 2);
//    for(int i = 1; i <= 2; i++)
//    {
//        QListWidgetItem * item = new QListWidgetItem(m_listWidget);
//        m_listWidget->setItemWidget(item, m_buttonGroup->button(i));
//        m_buttonGroup->button(i)->setIconSize(QSize(6, 6));
//    }
//}

//void DLeftSideBar::initCommonFolderBar()
//{
//    m_desktopButton = new DCheckableButton(":/icons/images/icons/folder-desktop-symbolic.svg", tr("Desktop"), this);
//    m_videoButton = new DCheckableButton(":/icons/images/icons/folder-videos-symbolic.svg", tr("Videos"), this);
//    m_musicButton = new DCheckableButton(":/icons/images/icons/folder-music-symbolic.svg", tr("Musics"), this);
//    m_pictureButton = new DCheckableButton(":/icons/images/icons/folder-pictures-symbolic.svg", tr("Pictures"), this);
//    m_docmentButton = new DCheckableButton(":/icons/images/icons/folder-documents-symbolic.svg", tr("Documents"), this);
//    m_downloadButton = new DCheckableButton(":/icons/images/icons/folder-download-symbolic.svg", tr("Downloads"), this);
//    m_trashButton = new DCheckableButton(":/icons/images/icons/user-trash-symbolic.svg", tr("Trash"), this);

//    m_buttonGroup->addButton(m_desktopButton, 3);
//    m_buttonGroup->addButton(m_videoButton, 4);
//    m_buttonGroup->addButton(m_pictureButton, 5);
//    m_buttonGroup->addButton(m_docmentButton, 6);
//    m_buttonGroup->addButton(m_downloadButton, 7);
//    m_buttonGroup->addButton(m_musicButton, 8);
//    m_buttonGroup->addButton(m_trashButton, 9);
//    for(int i = 3; i <= 9; i++)
//    {
//        QListWidgetItem * item = new QListWidgetItem(m_listWidget);
//        m_listWidget->setItemWidget(item, m_buttonGroup->button(i));
//        m_buttonGroup->button(i)->setIconSize(QSize(0, 0));
//    }
//}

//void DLeftSideBar::initDiskBar()
//{
//    m_computerButton = new DCheckableButton(":/icons/images/icons/drive-removable-media-symbolic.svg", tr("Disks"), this);
//    m_favoriteButton = new DCheckableButton(":/icons/images/icons/user-bookmarks-symbolic.svg", tr("Bookmarks"), this);
//    m_myMobileButton = new DCheckableButton(":/images/images/dark/appbar.iphone.png", tr("My Mobile"), this);

//    m_buttonGroup->addButton(m_computerButton, 10);
//    m_buttonGroup->addButton(m_favoriteButton, 11);
//    m_buttonGroup->addButton(m_myMobileButton, 12);
//    for(int i = 10; i <= 12; i++)
//    {
//        QListWidgetItem * item = new QListWidgetItem(m_listWidget);
//        m_listWidget->setItemWidget(item, m_buttonGroup->button(i));
//        m_buttonGroup->button(i)->setIconSize(QSize(16, 16));
//    }
//}

//void DLeftSideBar::initNetWorkBar()
//{
//    m_networkBar = new QFrame(this);
//}

void DLeftSideBar::initConnect()
{
    connect(m_buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(handleLocationChanged(int)));
    connect(m_tightNavButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(handleLocationChanged(int)));
    connect(m_fileButton, &DCheckableButton::released, this, &DLeftSideBar::toTightNav);
    connect(m_tightNavFileButton, &DCheckableButton::released, this, &DLeftSideBar::toNormalNav);
}

void DLeftSideBar::initTightNav()
{
    m_tightNav = new QFrame(this);
    m_tightNavFileButton = new DCheckableButton(":/icons/images/icons/sidebar_expand_normal.png", "");
    m_tightNavFileButton->setFixedHeight(40);
    m_tightNavButtonGroup = new QButtonGroup;
    QVBoxLayout * tightNavLayout = new QVBoxLayout;
    tightNavLayout->addWidget(m_tightNavFileButton);
    tightNavLayout->setContentsMargins(0, 0, 0, 0);
    tightNavLayout->setSpacing(0);
    m_tightNav->setLayout(tightNavLayout);
    m_tightNavButtonGroup->addButton(m_tightNavFileButton, 0);

    QListWidget * list = new QListWidget;
    list->setObjectName("ListWidget");
    for(int i = 1; i < m_iconlist.size(); i++)
    {
        DCheckableButton * button = new DCheckableButton(m_iconlist.at(i), "");
        QListWidgetItem * item = new QListWidgetItem(list);
        item->setSizeHint(QSize(30, 30));
        list->setItemWidget(item, button);
        m_tightNavButtonGroup->addButton(button, i);
    }
    tightNavLayout->addWidget(list);
}

void DLeftSideBar::initNav()
{
    m_nav = new QFrame;
    QVBoxLayout* navLayout = new QVBoxLayout;
    m_nav->setLayout(navLayout);
    //add file button
    m_fileButton = new DCheckableButton(":/icons/images/icons/sidebar_expand_normal.png", tr("File"));
    m_fileButton->setFixedHeight(40);
    navLayout->addWidget(m_fileButton);
    navLayout->setSpacing(0);
    navLayout->setContentsMargins(0, 0, 0, 0);
    m_buttonGroup = new QButtonGroup;
    m_buttonGroup->addButton(m_fileButton, 0);
    m_listWidget = new QListWidget;
    m_listWidget->setObjectName("ListWidget");
    QScrollBar * scrollbar = new QScrollBar;
    scrollbar->setObjectName("DScrollBar");
    m_listWidget->setVerticalScrollBar(scrollbar);
    m_listWidget->setAutoScroll(true);

    //recent, home, desktop, video, pcitures, documents, download, musics, trash
    for(int i = 1; i <= 9; i++)
    {
        DCheckableButton * button = new DCheckableButton(m_iconlist.at(i), m_nameList.at(i));
        QListWidgetItem * item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(110, 30));
        m_listWidget->setItemWidget(item, button);
        m_buttonGroup->addButton(button, i);
    }

    //add separator line
    QListWidgetItem * item = new QListWidgetItem(m_listWidget);
    item->setFlags(Qt::NoItemFlags);
    item->setSizeHint(QSize(110, 3));
    m_listWidget->setItemWidget(item, new DHorizSeparator);

    //disk, my mobile
    for(int i = 10; i < m_iconlist.size() - 1; i++)
    {
        DCheckableButton * button = new DCheckableButton(m_iconlist.at(i), m_nameList.at(i));
        QListWidgetItem * item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(110, 30));
        m_listWidget->setItemWidget(item, button);
        m_buttonGroup->addButton(button, i);
    }

    //add separator line
    item = new QListWidgetItem(m_listWidget);
    item->setFlags(Qt::NoItemFlags);
    item->setSizeHint(QSize(110, 3));
    m_listWidget->setItemWidget(item, new DHorizSeparator);

    //add bookmark item
    int i = m_iconlist.size() - 1;
    DCheckableButton * button = new DCheckableButton(m_iconlist.at(i), m_nameList.at(i));
    item = new QListWidgetItem(m_listWidget);
    item->setSizeHint(QSize(110, 30));
    m_listWidget->setItemWidget(item, button);
    m_buttonGroup->addButton(button, i);

    for(int i = 12; i < 30; i++)
    {
        DCheckableButton * button = new DCheckableButton(":/images/images/dark/appbar.app.favorite.png", "label");
        QListWidgetItem * item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(110, 30));
        m_listWidget->setItemWidget(item, button);
        m_buttonGroup->addButton(button, i);
    }
    navLayout->addWidget(m_listWidget);
}

QString DLeftSideBar::getStandardPathbyId(int id)
{
    QString path;
    switch (id) {
    case 1:
        path = Recent;
        break;
    case 2:
        path = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
        break;
    case 3:
        path = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0);
        break;
    case 4:
        path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).at(0);
        break;
    case 5:
        path = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0);
        break;
    case 6:
        path = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0);
        break;
    case 7:
        path = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).at(0);
        break;
    case 8:
        path = QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0);
        break;
    case 9:
        path = TrashDir;
        break;
    case 10:
        path = QDir::rootPath();
        break;
    default:
        break;
    }
    return path;
}

void DLeftSideBar::handleLocationChanged(int id)
{
    if (id == 1){

    }else{
        QString path = getStandardPathbyId(id);
        emit fileSignalManager->currentUrlChanged(path);
    }
}

void DLeftSideBar::toTightNav()
{
    m_stackedWidget->setCurrentIndex(0);
    this->setFixedWidth(60);
}

void DLeftSideBar::toNormalNav()
{
    m_stackedWidget->setCurrentIndex(1);
    this->setFixedWidth(160);
}


