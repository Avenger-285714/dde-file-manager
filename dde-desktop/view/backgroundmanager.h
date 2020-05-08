#ifndef BACKGROUNDMANAGER_H
#define BACKGROUNDMANAGER_H

#include "backgroundwidget.h"
#include "screen/abstractscreen.h"

#include <com_deepin_wm.h>
#include <DWindowManagerHelper>
#include <dgiosettings.h>

#include <QObject>
#include <QMap>

using WMInter = com::deepin::wm;

DGUI_USE_NAMESPACE
class AbstractScreen;
class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundManager(bool preview = false,QObject *parent = nullptr);
    ~BackgroundManager();
    bool isEnabled() const;
    void setVisible(bool visible);
    bool isVisible() const;
    BackgroundWidgetPointer backgroundWidget(ScreenPointer) const;
    inline QMap<ScreenPointer,BackgroundWidgetPointer> allbackgroundWidgets() const{return m_backgroundMap;}
    //自定义背景，临时方案
    void setBackgroundImage(const QString &screen,const QString &path);
signals:
    void sigBackgroundBuilded(int mode); //通知canvasview
public slots:
    void onBackgroundBuild();       //创建背景窗口
    void onSkipBackgroundBuild();   //不创建背景窗口，直接发完成信号
    void onResetBackgroundImage();
protected slots:
    void onRestBackgroundManager(); //重置背景，响应窗管改变
    void onScreenGeometryChanged(ScreenPointer);    //响应屏幕大小改变
private:
    void init();
    BackgroundWidgetPointer createBackgroundWidget(ScreenPointer);
protected:
    DGioSettings *gsettings = nullptr;
    WMInter *wmInter = nullptr;
    DWindowManagerHelper* windowManagerHelper = nullptr;
private:
    bool m_preview = false; //壁纸预览
    bool m_visible = true;
    int currentWorkspaceIndex = 0;

    QMap<ScreenPointer,BackgroundWidgetPointer> m_backgroundMap;

    //记录设置的背景的壁纸，临时解决方案
    QMap<QString, QString> m_backgroundImagePath;

};

#endif // BACKGROUNDMANAGER_H
