#ifndef DFMOPTICALMEDIAWIDGET_H
#define DFMOPTICALMEDIAWIDGET_H

#include "app/define.h"
#include "dfileservices.h"

#include <QWidget>

DFM_BEGIN_NAMESPACE
class DFileStatisticsJob;
DFM_END_NAMESPACE
DFM_USE_NAMESPACE

class DFMOpticalMediaWidgetPrivate;
class DFMOpticalMediaWidget : public QWidget
{
    Q_OBJECT
public:
    //fix: 光盘容量属性: 光盘容量状态
    enum BurnCapacityStatusAttribute {
        BCSA_BurnCapacityStatusEjct =  0, //光盘容量状态：0,光驱弹出状态
        BCSA_BurnCapacityStatusAdd, //光盘容量状态：1,光驱弹入处于添加未挂载状态
        BCSA_BurnCapacityStatusAddMount //光盘容量状态：2,光驱弹入处于添加后并挂载的状态
    };

    //fixed:CD display size error
    static quint64 g_totalSize;
    static quint64 g_usedSize;
    //fix: 动态获取刻录选中文件的字节大小
    static qint64 g_selectBurnFilesSize;
    static qint64 g_selectBurnDirFileCount;

    explicit DFMOpticalMediaWidget(QWidget* parent);
    ~DFMOpticalMediaWidget();

    void updateDiscInfo(QString dev);
    //fix: 设置光盘容量属性
    static void setBurnCapacity(int status);

    //fix: 动态更新光驱磁盘状态
private slots:
    void selectBurnFilesOptionUpdate();

private:
    QScopedPointer<DFMOpticalMediaWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DFMOpticalMediaWidget)
    DFileStatisticsJob *m_pStatisticWorker = nullptr;
};

#endif // DFMOPTICALMEDIAWIDGET_H
