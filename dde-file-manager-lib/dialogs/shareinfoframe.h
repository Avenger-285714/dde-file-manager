#ifndef SHAREINFOFRAME_H
#define SHAREINFOFRAME_H

#include <QFrame>
#include <QTimer>

#include <dcheckbox.h>
#include <dlineedit.h>

#include "dabstractfileinfo.h"

class QComboBox;

DWIDGET_USE_NAMESPACE

class ShareInfoFrame : public QFrame
{
    Q_OBJECT
public:
    explicit ShareInfoFrame(const DAbstractFileInfoPointer &info, QWidget *parent = 0);
    ~ShareInfoFrame();

    void initUI();
    void initConnect();
    void setFileinfo(const DAbstractFileInfoPointer &fileinfo);

signals:
    void folderShared(const QString& filePath);

public slots:
    void handleCheckBoxChanged(int state);
    void handleShareNameChanged(const QString& name);
    void handlePermissionComboxChanged(int index);
    void handleAnonymityComboxChanged(int index);
    void handShareInfoChanged();
    void doShaeInfoSetting();
    void updateShareInfo(const QString& filePath);

private:
    DAbstractFileInfoPointer m_fileinfo;
    QCheckBox* m_sharCheckBox = NULL;
    DLineEdit* m_shareNamelineEdit = NULL;
    QComboBox* m_permissoComBox = NULL;
    QComboBox* m_anonymityCombox = NULL;
    QTimer* m_jobTimer;
};

#endif // SHAREINFOFRAME_H
