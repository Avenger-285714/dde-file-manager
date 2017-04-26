#ifndef DIALOGMANAGER_H
#define DIALOGMANAGER_H

#include <QObject>
#include <QMap>

#include "durl.h"

class DTaskDialog;
class FileJob;
class DAbstractFileInfo;
class DUrl;
class DFMEvent;
class DFMUrlListBaseEvent;
class PropertyDialog;
class CloseAllDialogIndicator;
class TrashPropertyDialog;
class ComputerPropertyDialog;
class QTimer;

class DialogManager : public QObject
{
    Q_OBJECT

public:
    explicit DialogManager(QObject *parent = 0);
    ~DialogManager();
    void initTaskDialog();
    void initCloseIndicatorDialog();
    void initConnect();
    QPoint getPerportyPos(int dialogWidth, int dialogHeight, int count, int index);
    bool isTaskDialogEmpty();
signals:

public slots:
    void handleConflictRepsonseConfirmed(const QMap<QString, QString> &jobDetail, const QMap<QString, QVariant> &response);
    void addJob(FileJob * job);
    void removeJob(const QString &jobId);
    void removeAllJobs();
    void updateJob();
    void startUpdateJobTimer();
    void stopUpdateJobTimer();

    void abortJob(const QMap<QString, QString> &jobDetail);
    void abortJobByDestinationUrl(const DUrl& url);

    void showCopyMoveToSelfDialog(const QMap<QString, QString> &jobDetail);

    void showUrlWrongDialog(const DUrl &url);
    int showRunExcutableScriptDialog(const DUrl& url, quint64 winId);
    int showRunExcutableFileDialog(const DUrl &url, quint64 winId);
    int showRenameNameSameErrorDialog(const QString& name, const DFMEvent &event);
    int showDeleteFilesClearTrashDialog(const DFMUrlListBaseEvent &event);
    int showRemoveBookMarkDialog(const DFMEvent &event);
    void showOpenWithDialog(const DFMEvent &event);
    void showPropertyDialog(const DFMEvent &event);
    void showShareOptionsInPropertyDialog(const DFMEvent &event);
    void showTrashPropertyDialog(const DFMEvent &event);
    void showComputerPropertyDialog();
    void showDevicePropertyDialog(const DFMEvent &event);
    void showDiskErrorDialog(const QString &id, const QString &errorText);
    void showBreakSymlinkDialog(const QString &targetName, const DUrl& linkfile);
    void showAboutDialog(quint64 winId);
    void showUserSharePasswordSettingDialog(quint64 winId);
    void showGlobalSettingsDialog(quint64 winId);
    void showDiskSpaceOutOfUsedDialog();
    void showFailToCreateSymlinkDialog(const QString &errorString);
    void showMoveToTrashConflictDialog(const DUrlList& urls);
    void showDeleteSystemPathWarnDialog(quint64 winId);

    void removePropertyDialog(const DUrl& url);
    void closeAllPropertyDialog();
    void updateCloseIndicator();
    void raiseAllPropertyDialog();
    void handleFocusChanged(QWidget* old, QWidget* now);

    void refreshPropertyDialogs(const DUrl& oldUrl, const DUrl& newUrl);

private:
    DTaskDialog* m_taskDialog = NULL;
    CloseAllDialogIndicator* m_closeIndicatorDialog;
    TrashPropertyDialog* m_trashDialog;
    ComputerPropertyDialog* m_computerDialog;
    QMap<QString, FileJob*> m_jobs;
    QMap<DUrl, PropertyDialog*> m_propertyDialogs;
    QTimer* m_closeIndicatorTimer = NULL;
    QTimer* m_updateJobTaskTimer = NULL;
};

#endif // DIALOGMANAGER_H
