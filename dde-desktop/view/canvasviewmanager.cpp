#include "canvasviewmanager.h"
#include "screen/screenhelper.h"
#include "util/xcb/xcb.h"
#include "presenter/gridmanager.h"
#include "desktopitemdelegate.h"
#include "dfilesystemmodel.h"

#include <QPair>

inline QRect relativeRect(const QRect &avRect,const QRect &geometry)
{
    QPoint relativePos = avRect.topLeft() - geometry.topLeft();

    return QRect(relativePos,avRect.size());
}

CanvasViewManager::CanvasViewManager(BackgroundManager *bmrg, QObject *parent)
    : QObject(parent)
    , m_background(bmrg)
{
    init();
}

CanvasViewManager::~CanvasViewManager()
{
    m_background = nullptr;
    m_canvasMap.clear();
}

void CanvasViewManager::onCanvasViewBuild(int imode)
{
    //屏幕模式判断
    AbstractScreenManager::DisplayMode mode = (AbstractScreenManager::DisplayMode)imode;

    GridManager::instance()->restCoord();
    //实际是单屏
    if ((AbstractScreenManager::Showonly == mode) || (AbstractScreenManager::Duplicate == mode) //仅显示和复制
            || (ScreenMrg->screens().count() == 1)){

        ScreenPointer primary = ScreenMrg->primaryScreen();
        CanvasViewPointer mView = m_canvasMap.value(primary);

        //删除其他
        m_canvasMap.clear();

        if (mView.get() == nullptr){
            mView = CanvasViewPointer(new CanvasGridView(primary->name()));
            mView->setScreenNum(1);
            GridManager::instance()->addCoord(1, {0,0});
            mView->show();
        }
        else {
            mView->setScreenNum(1);
            GridManager::instance()->addCoord(1, {0,0});
        }

        GridManager::instance()->setDisplayMode(true);

        mView->initRootUrl();
        m_canvasMap.insert(primary, mView);

        qDebug() << "mode" << mode << mView->geometry() << primary->availableGeometry()<< primary->geometry()
                 << primary->name() << "num" << 1 << "devicePixelRatio" << ScreenMrg->devicePixelRatio();;
    }
    else {
        auto currentScreens = ScreenMrg->logicScreens();
        int screenNum = 0;
        //检查新增的屏幕
        for (const ScreenPointer &sp : currentScreens){
            ++screenNum;
            CanvasViewPointer mView = m_canvasMap.value(sp);

            //新增
            if (mView.get() == nullptr){
                mView = CanvasViewPointer(new CanvasGridView(sp->name()));
                mView->setScreenNum(screenNum);
                GridManager::instance()->addCoord(screenNum, {0,0});

                mView->show();
                mView->initRootUrl();
                m_canvasMap.insert(sp, mView);
            }
            else {
                GridManager::instance()->addCoord(screenNum, {0,0});
                mView->setScreenNum(screenNum);
            }

            qDebug() << "mode" << mode << mView->geometry() <<sp->availableGeometry()<< sp->geometry()
                     << sp->name() << "num" << screenNum
                     << "devicePixelRatio" << ScreenMrg->devicePixelRatio();;
        }

        //检查移除的屏幕
        for (const ScreenPointer &sp : m_canvasMap.keys()){
            if (!currentScreens.contains(sp)){
                m_canvasMap.remove(sp);
                qDebug() << "mode" << mode << "remove" << sp->name()
                         << sp->geometry();
            }
        }
        GridManager::instance()->setDisplayMode(false);
    }

    onBackgroundEnableChanged();
}

void CanvasViewManager::onBackgroundEnableChanged()
{
    if (m_background->isEnabled()) {
        for (const ScreenPointer &sp : m_canvasMap.keys()){

            CanvasViewPointer mView = m_canvasMap.value(sp);
            BackgroundWidgetPointer bw = m_background->backgroundWidget(sp);
            mView->setAttribute(Qt::WA_NativeWindow, false);
            bw->setView(mView);
            QRect avRect;
            if (sp == ScreenMrg->primaryScreen()){
                avRect = relativeRect(sp->availableGeometry(),sp->geometry());
            }
            else {
                avRect = relativeRect(sp->geometry(),sp->geometry());
            }
            qDebug() << mView << avRect << sp->geometry() << sp->availableGeometry();
            mView->show();
            mView->setGeometry(avRect);
        }
    }
    else {
        for (const ScreenPointer &sp : m_canvasMap.keys()){

            CanvasViewPointer mView = m_canvasMap.value(sp);
            mView->setParent(nullptr);
            mView->setWindowFlag(Qt::FramelessWindowHint, true);
            Xcb::XcbMisc::instance().set_window_type(mView->winId(), Xcb::XcbMisc::Desktop);
            mView->show();
            mView->setGeometry(sp == ScreenMrg->primaryScreen() ? sp->availableGeometry() : sp->geometry());
            qDebug() << "rrrrrrrrrrrrrrr" << ScreenMrg->primaryScreen()->name() << mView->geometry() << sp->name() << sp->geometry() << sp->availableGeometry();
        }
    }
    GridManager::instance()->initGridItemsInfos();
}

void CanvasViewManager::onScreenGeometryChanged(ScreenPointer sp)
{
    CanvasViewPointer mView = m_canvasMap.value(sp);
    if (mView.get() != nullptr){
        QRect avRect;
        if (m_background->isEnabled()) {
            if (sp == ScreenMrg->primaryScreen()){
                avRect = relativeRect(sp->availableGeometry(), sp->geometry());
            }
            else {
                avRect = relativeRect(sp->geometry(), sp->geometry());
            }
        }
        else {
            avRect = sp == ScreenMrg->primaryScreen() ? sp->availableGeometry() : sp->geometry();
        }

        qDebug() << "dddddddddddddd" << ScreenMrg->primaryScreen()->name()
                 << mView->geometry() << sp->name()
                 << sp->geometry() << sp->availableGeometry() << avRect;
        mView->setGeometry(avRect);
    }
}

void CanvasViewManager::onSyncOperation(int so,QVariant var)
{
    GridManager::SyncOperation type = (GridManager::SyncOperation)so;
    qDebug() << "sync type" << type << "data" << var;

    switch (type) {
    case GridManager::soAutoMerge:{
        bool enable = var.toBool();
        for (CanvasViewPointer view : m_canvasMap.values()){
            view->setAutoMerge(enable);
        }

        if (!enable)
            GridManager::instance()->initGridItemsInfos();
        break;
    }
    case GridManager::soRename:{ //处理自动排列时右键新建文件，编辑框显示问题
        QString file = var.toString();
        arrageEditDeal(file);
        break;
    }
    case GridManager::soIconSize:{  //处理图标大小
        int level = var.toInt();
        for (CanvasViewPointer view : m_canvasMap.values()){
            view->syncIconLevel(level);
        }
        break;
    }
    case GridManager::soSort:{
        QPoint sort = var.toPoint();
        for (CanvasViewPointer view : m_canvasMap.values()){
            view->model()->setSortRole(sort.x(),(Qt::SortOrder)sort.y());
            view->update();
        }
        break;
    }
    case GridManager::soHideEditing:{   //隐藏文件移动后留下编辑框
        for (CanvasViewPointer view : m_canvasMap.values()){
            view->itemDelegate()->hideNotEditingIndexWidget();
            view->update();
        }
        break;
    }
    case GridManager::soUpdate:{
        for (CanvasViewPointer view : m_canvasMap.values()){
            view->update();
        }
        break;
    }
    case GridManager::soAutoMergeUpdate:{
        qDebug() << "update when canvas folder expand changed";
        auto mergeMap = var.value<QMap<QString,DUrl>>();
        if(mergeMap.isEmpty())
            return;
        auto one = mergeMap.begin();

        for (CanvasViewPointer view : m_canvasMap.values()){
            if(one.key() == view->canvansScreenName())
                continue;
            view->updateEntryExpandedState(one.value());
        }
        break;
    }
    default:
        break;
    }
}

void CanvasViewManager::onSyncSelection(CanvasGridView *v, DUrlList selected)
{
    disconnect(GridManager::instance(), &GridManager::sigSyncSelection,
            this,&CanvasViewManager::onSyncSelection);
    //qDebug() << "sync selection" << v->canvansScreenName() << selected.size();
    for (CanvasViewPointer view : m_canvasMap.values()) {
        if (view == v)
            continue;
        view->select(selected);
        view->update();
    }
    connect(GridManager::instance(), &GridManager::sigSyncSelection,
            this,&CanvasViewManager::onSyncSelection,Qt::DirectConnection);
}

void CanvasViewManager::arrageEditDeal(const QString &file)
{
    QPair<int, QPoint> orgPos;
    //找文件在哪个屏上
    if (GridManager::instance()->find(file,orgPos)){
        for (CanvasViewPointer view : m_canvasMap.values()) {
            //绘制屏上，打开编辑框
            if (view->screenNum() == orgPos.first){
                //已有editor，跳过
                if (view->itemDelegate()->editingIndexWidget()){
                    qDebug() << "has editor" << view->itemDelegate()->editingIndex();
                    continue;
                }

                //激活编辑框
                DUrl fileUrl(file);
                auto index = view->model()->index(fileUrl);
                view->select(QList<DUrl>() << fileUrl);
                bool bEdit = view->edit(index,QAbstractItemView::EditKeyPressed,0);
                QWidget *editor = view->itemDelegate()->editingIndexWidget();
                if (editor)
                    editor->activateWindow();
            }
        }
    }
}

void CanvasViewManager::init()
{
    //屏幕增删，模式改变
    connect(m_background,&BackgroundManager::sigBackgroundBuilded
            , this,&CanvasViewManager::onCanvasViewBuild);

    //屏幕大小改变
    connect(ScreenHelper::screenManager(), &AbstractScreenManager::sigScreenGeometryChanged,
            this, &CanvasViewManager::onScreenGeometryChanged);

    //可用区改变
    connect(ScreenHelper::screenManager(), &AbstractScreenManager::sigScreenAvailableGeometryChanged,
            this, &CanvasViewManager::onScreenGeometryChanged);

    //grid改变
    connect(GridManager::instance(), &GridManager::sigSyncOperation,
            this, &CanvasViewManager::onSyncOperation,Qt::QueuedConnection);

    //同步选中状态
    connect(GridManager::instance(), &GridManager::sigSyncSelection,
            this,&CanvasViewManager::onSyncSelection,Qt::DirectConnection);

    onCanvasViewBuild(ScreenMrg->displayMode());
}
