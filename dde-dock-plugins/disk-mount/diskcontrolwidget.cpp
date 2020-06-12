/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *               2016 ~ 2018 dragondjf
 *
 * Author:     dragondjf<dingjiangfeng@deepin.com>
 *
 * Maintainer: dragondjf<dingjiangfeng@deepin.com>
 *             zccrs<zhangjide@deepin.com>
 *             Tangtong<tangtong@deepin.com>
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

#include "diskcontrolwidget.h"
#include "diskcontrolitem.h"
#include "dattachedudisks2device.h"
#include "dattachedvfsdevice.h"
#include "models/dfmrootfileinfo.h"

#include <dgiovolumemanager.h>
#include <dgiomount.h>
#include <dgiofile.h>

#include <ddiskmanager.h>
#include <dblockdevice.h>
#include <ddiskdevice.h>
#include <dfmsettings.h>
#include <dgiosettings.h>
#include <DDesktopServices>

#include <QDebug>
#include <QProcess>
#include <QThreadPool>
#include <QtConcurrent>
#include <QScrollBar>
#include <QDebug>
#include <DDBusSender>
#define WIDTH           300

DWIDGET_USE_NAMESPACE

DFM_USE_NAMESPACE

DiskControlWidget::DiskControlWidget(QWidget *parent)
    : QScrollArea(parent),
      m_centralLayout(new QVBoxLayout),
      m_centralWidget(new QWidget)
{
    this->setObjectName("DiskControlWidget-QScrollArea");

    m_centralWidget->setLayout(m_centralLayout);
    m_centralWidget->setFixedWidth(WIDTH);

    m_vfsManager.reset(new DGioVolumeManager);

    setWidget(m_centralWidget);
    setFixedWidth(WIDTH);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    verticalScrollBar()->setSingleStep(7);
    m_centralWidget->setAutoFillBackground(false);
    viewport()->setAutoFillBackground(false);

    m_diskManager = new DDiskManager(this);
    initConnect();
}

DiskControlWidget::~DiskControlWidget()
{

}

void DiskControlWidget::initConnect()
{
    connect(m_diskManager, &DDiskManager::diskDeviceAdded, this, [this](const QString & path) {
        // blumia: Workaround. Wait for udisks2 add new device to device list.
        QTimer::singleShot(500, this, [ = ]() {
            onDriveConnected(path);
        });
    });
    connect(m_diskManager, &DDiskManager::diskDeviceRemoved, this, &DiskControlWidget::onDriveDisconnected);
    connect(m_diskManager, &DDiskManager::mountAdded, this, &DiskControlWidget::onMountAdded);
    connect(m_diskManager, &DDiskManager::mountRemoved, this, &DiskControlWidget::onMountRemoved);
    connect(m_diskManager, &DDiskManager::fileSystemAdded, this, &DiskControlWidget::onVolumeAdded);
    connect(m_diskManager, &DDiskManager::fileSystemRemoved, this, &DiskControlWidget::onVolumeRemoved);

    connect(m_vfsManager.data(), &DGioVolumeManager::mountAdded, this, &DiskControlWidget::onVfsMountChanged);
//    connect(m_vfsManager.data(), &DGioVolumeManager::mountChanged, this, &DiskControlWidget::onVfsMountChanged);
    connect(m_vfsManager.data(), &DGioVolumeManager::mountRemoved, this, &DiskControlWidget::onVfsMountChanged);
}

void DiskControlWidget::startMonitor()
{
    m_diskManager->setWatchChanges(true);
    onDiskListChanged();
}

/*
 *
 * TODO: move this thing into dtkcore or somewhere...
 *       There is also a FileUtils::getKernelParameters() in dde-file-manager.
 * blumia: for writing unit test, try validate the result with `tr ' ' '\n' < /proc/cmdline`
 */
QMap<QString, QString> getKernelParameters()
{
    QFile cmdline("/proc/cmdline");
    cmdline.open(QIODevice::ReadOnly);
    QByteArray content = cmdline.readAll();

    QByteArrayList paraList(content.split(' '));

    QMap<QString, QString> result;
    result.insert("_ori_proc_cmdline", content);

    for (const QByteArray &onePara : paraList) {
        int equalsIdx = onePara.indexOf('=');
        QString key = equalsIdx == -1 ? onePara.trimmed() : onePara.left(equalsIdx).trimmed();
        QString value = equalsIdx == -1 ? QString() : onePara.right(equalsIdx).trimmed();
        result.insert(key, value);
    }

    return result;
}

void DiskControlWidget::doStartupAutoMount()
{
    // check if we are in live system, don't do auto mount if we are in live system.
    static QMap<QString, QString> cmdline = getKernelParameters();
    if (cmdline.value("boot", "") == QStringLiteral("live")) {
        autoMountDisabled = true;
        return;
    }

    if (getGsGlobal()->value("GenericAttribute", "AutoMount", false).toBool() == false) {
        return;
    }


    bool iswWayland = false;
    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
    QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

    if (XDG_SESSION_TYPE == QLatin1String("wayland") ||
            WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        iswWayland = true;
    }

    QStringList blDevList = m_diskManager->blockDevices();

    for (const QString &blDevStr : blDevList) {
        QScopedPointer<DBlockDevice> blDev(DDiskManager::createBlockDevice(blDevStr));

        if (iswWayland && blDevStr.contains(QRegularExpression("/sd[a-c][1-9]*$"))) {
            continue;
        }

        if (blDev->isEncrypted()) continue;
        if (blDev->hintIgnore()) continue;

        QList<QByteArray> mountPoints = blDev->mountPoints();
        if (blDev->hasFileSystem() && blDev->mountPoints().isEmpty()) {
            blDev->mount({{"auth.no_user_interaction", true}});
        }
    }

    qDebug() << "call desktop.canvas.reFresh";
    // call desktop.canvas.reFresh
    DDBusSender()
    .service("com.deepin.dde.desktop")
    .path("/com/deepin/dde/desktop/canvas")
    .interface("com.deepin.dde.desktop.Canvas")
    .method(QString("Refresh")).call();
}

bool isProtectedDevice(DBlockDevice *blk)
{
    DGioSettings gsettings("com.deepin.dde.dock.module.disk-mount", "/com/deepin/dde/dock/module/disk-mount/");
    if (gsettings.value("protect-non-media-mounts").toBool()) {
        QList<QByteArray> mountPoints = blk->mountPoints();
        for (auto &mountPoint : mountPoints) {
            if (!mountPoint.startsWith("/media/")) {
                return true;
            }
        }
    }

    if (gsettings.value("protect-root-device-mounts").toBool()) {
        QStorageInfo qsi("/");
        QStringList rootDevNodes = DDiskManager::resolveDeviceNode(qsi.device(), {});
        if (!rootDevNodes.isEmpty()) {
            if (DDiskManager::createBlockDevice(rootDevNodes.first())->drive() == blk->drive()) {
                return true;
            }
        }
    }

    return false;
}

void DiskControlWidget::unmountAll()
{
    QStringList blockDevices = m_diskManager->blockDevices();

    QtConcurrent::run([blockDevices]() {
        for (const QString &blDevStr : blockDevices) {
            QScopedPointer<DBlockDevice> blDev(DDiskManager::createBlockDevice(blDevStr));
            if (isProtectedDevice(blDev.data())) continue;
            if (blDev->hasFileSystem() /* && DFMSetting*/ && !blDev->mountPoints().isEmpty() && !blDev->hintIgnore() && !blDev->hintSystem()) {
                QScopedPointer<DDiskDevice> diskDev(DDiskManager::createDiskDevice(blDev->drive()));
                blDev->unmount({});
                qDebug() << "unmountAll" << "removable" <<  diskDev->removable() <<
                         "optical" << diskDev->optical() <<
                         "canPowerOff" << diskDev->canPowerOff() <<
                         "ejectable" << diskDev->ejectable();

                if (diskDev->removable()) {
                    diskDev->eject({});
                    qDebug() << "unmountAll";
                    if (diskDev->lastError().isValid()) {
                        qWarning() << diskDev->lastError().name() << blockDevices;
                        NotifyMsg(tr("Disk is busy, cannot eject now"));
                    }
                }
                if (diskDev->optical()) { // is optical
                    if (diskDev->ejectable()) {
                        diskDev->eject({});
                        if (diskDev->lastError().isValid()) {
                            qWarning() << diskDev->lastError().name() << blockDevices;
                            NotifyMsg(tr("Disk is busy, cannot eject now"));
                        }
                        return;
                    }
                }

                if (diskDev->canPowerOff()) {
                    diskDev->powerOff({});
                }
            }
        }
    });

    QList<QExplicitlySharedDataPointer<DGioMount> > vfsMounts = getVfsMountList();
    for (auto mount : vfsMounts) {
        if (mount->isShadowed()) {
            continue;
        }
        QExplicitlySharedDataPointer<DGioFile> rootFile = mount->getRootFile();
        QString path = rootFile->path();
        DAttachedVfsDevice *dad = new DAttachedVfsDevice(path);
        if (dad->isValid()) {
            dad->detach();
        } else {
            qDebug() << "dad->isValid()" << mount->name();
        }
    }
}

const QList<QExplicitlySharedDataPointer<DGioMount> > DiskControlWidget::getVfsMountList()
{
    QList<QExplicitlySharedDataPointer<DGioMount> > result;
    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = m_vfsManager->getMounts();
    for (auto mount : mounts) {
        QExplicitlySharedDataPointer<DGioFile> file = mount->getRootFile();
        QString uriStr = file->uri();
        QUrl url(uriStr);

#ifdef QT_DEBUG
        if (!url.isValid()) {
            qWarning() << "Gio uri is not a vaild QUrl!" << uriStr;
            qFatal("See the above warning for reason");
        }
#endif // QT_DEBUG

        if (url.scheme() == "file") continue;

        result.append(mount);
    }

    return result;
}

void DiskControlWidget::onDiskListChanged()
{
    while (QLayoutItem *item = m_centralLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    int mountedCount = 0;

    QStringList blDevList = DDiskManager::blockDevices({});
    for (const QString &blDevStr : blDevList) {
        QScopedPointer<DBlockDevice> blDev(DDiskManager::createBlockDevice(blDevStr));
        if (blDev->hasFileSystem() && !blDev->mountPoints().isEmpty() && !blDev->hintSystem() && !blDev->hintIgnore()) {
            if (isProtectedDevice(blDev.data())) continue;
            mountedCount++;
            QStringList blDevStrArray = blDevStr.split(QDir::separator());
            QString tagName = blDevStrArray.isEmpty() ? "" : blDevStrArray.last();
            DAttachedUdisks2Device *dad = new DAttachedUdisks2Device(blDev.data());
            qDebug() << "create new item, tagname is" << tagName;
            DiskControlItem *item = new DiskControlItem(dad, this);
            item->setTagName(tagName);

            class ErrHandle : public ErrorHandleInfc, public QObject
            {
            public:
                ErrHandle(QObject *parent): QObject(parent) {}
                virtual void onError(DAttachedDeviceInterface *device)
                {
                    DAttachedUdisks2Device *drv = dynamic_cast<DAttachedUdisks2Device *>(device);
                    if (drv) {
                        qWarning() << drv->blockDevice()->lastError().name() << device->displayName();
                        NotifyMsg(DiskControlWidget::tr("Disk is busy, cannot eject now"));
                    }
                }
            };
            dad->setErrorHandler(new ErrHandle(item));

            m_centralLayout->addWidget(item);
        }
    }

    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = getVfsMountList();
    for (auto mount : mounts) {
        if (mount->isShadowed()) {
            continue;
        }
        QExplicitlySharedDataPointer<DGioFile> rootFile = mount->getRootFile();
        QString path = rootFile->path();
        DAttachedVfsDevice *dad = new DAttachedVfsDevice(path);
        if (dad->isValid()) {
            mountedCount++;
            DiskControlItem *item = new DiskControlItem(dad, this);
            m_centralLayout->addWidget(item);
        } else {
            delete dad;
        }
    }

    emit diskCountChanged(mountedCount);

    const int contentHeight = mountedCount * 70;
    const int maxHeight = std::min(contentHeight, 70 * 6);

    m_centralWidget->setFixedHeight(contentHeight);
    setFixedHeight(maxHeight);

    verticalScrollBar()->setPageStep(maxHeight);
    verticalScrollBar()->setMaximum(contentHeight - maxHeight);
}

void DiskControlWidget::onDriveConnected(const QString &deviceId)
{
    QScopedPointer<DDiskDevice> diskDevice(DDiskManager::createDiskDevice(deviceId));
    if (diskDevice->removable()) {
        DDesktopServices::playSystemSoundEffect("device-added");

        if (autoMountDisabled) {
            return;
        }

        bool mountAndOpen = false;

        // Check if we need do auto mount..
        getGsGlobal()->reload();
        if (getGsGlobal()->value("GenericAttribute", "AutoMountAndOpen", false).toBool()) {
            // mount and open
            mountAndOpen = true;
        } else if (getGsGlobal()->value("GenericAttribute", "AutoMount", false).toBool()) {
            // mount
            // no flag there..
        } else {
            // no need to do auto mount, return.
            return;
        }

        QDBusInterface loginManager("org.freedesktop.login1",
                                    "/org/freedesktop/login1/user/self",
                                    "org.freedesktop.login1.User",
                                    QDBusConnection::systemBus());
        QVariant replay = loginManager.property(("State"));
        if (replay.isValid()) {
            QString state = replay.toString();
            if (state != "active") {
                return;
            }
        }

        // Do auto mount stuff..
        QStringList blDevList = DDiskManager::blockDevices({});
        for (const QString &blDevStr : blDevList) {
            QScopedPointer<DBlockDevice> blDev(DDiskManager::createBlockDevice(blDevStr));

            if (isProtectedDevice(blDev.data())) continue;
            if (blDev->drive() != deviceId) continue;
            if (blDev->isEncrypted()) continue;
            if (blDev->hintIgnore()) continue;

            if (blDev->hasFileSystem() && blDev->mountPoints().isEmpty()) {

                // blumia: if mount&open enabled and dde-file-manager also got installed, use dde-file-manager.
                //         using mount scheme with udisks sub-scheme to give user a *device is mounting* feedback.
                if (mountAndOpen && !QStandardPaths::findExecutable(QStringLiteral("dde-file-manager")).isEmpty()) {
                    QString mountUrlStr = DFMROOT_ROOT + blDevStr.mid(QString("/org/freedesktop/UDisks2/block_devices/").length()) + "." SUFFIX_UDISKS;
                    QProcess::startDetached(QStringLiteral("dde-file-manager"), {mountUrlStr});
                    return;
                }

                QString mountPoint = blDev->mount({});
                if (mountAndOpen && !mountPoint.isEmpty()) {
                    DDesktopServices::showFolder(QUrl::fromLocalFile(mountPoint));
                }
            }
        }
    }
}

void DiskControlWidget::onDriveDisconnected()
{
    DDesktopServices::playSystemSoundEffect("device-removed");
    NotifyMsg(QObject::tr("Device has been removed"));
    onDiskListChanged();
}

void DiskControlWidget::onMountAdded()
{
    onDiskListChanged();
}

void DiskControlWidget::onMountRemoved(const QString &blockDevicePath, const QByteArray &mountPoint)
{
    Q_UNUSED(mountPoint);
    QScopedPointer<DBlockDevice> blDev(DDiskManager::createBlockDevice(blockDevicePath));
    if (blDev) {
        QScopedPointer<DDiskDevice> diskDev(DDiskManager::createDiskDevice(blDev->drive()));
        if (diskDev && diskDev->removable()) {
            qDebug() << "removable device" << blockDevicePath;// << mountPoint;
            //return; // removable device emit onDiskListChanged too
        }
    }

    qDebug() << "unmounted," << mountPoint;
    onDiskListChanged();
}

void DiskControlWidget::onVolumeAdded()
{
    onDiskListChanged();
}

void DiskControlWidget::onVolumeRemoved()
{
    onDiskListChanged();
}

void DiskControlWidget::onVfsMountChanged(QExplicitlySharedDataPointer<DGioMount> mount)
{
    QExplicitlySharedDataPointer<DGioFile> file = mount->getRootFile();
    QString uriStr = file->uri();
    QUrl url(uriStr);

#ifdef QT_DEBUG
    if (!url.isValid()) {
        qWarning() << "Gio uri is not a vaild QUrl!" << uriStr;
        qFatal("See the above warning for reason");
    }
#endif // QT_DEBUG

    if (url.scheme() == "file") return;

    onDiskListChanged();
}

void DiskControlWidget::unmountDisk(const QString &diskId) const
{
    QtConcurrent::run([diskId]() {
        QScopedPointer<DBlockDevice> blDev(DDiskManager::createBlockDevice(diskId));
        QScopedPointer<DDiskDevice> diskDev(DDiskManager::createDiskDevice(blDev->drive()));
        blDev->unmount({});
        if (diskDev->optical()) { // is optical
            if (diskDev->ejectable()) {
                diskDev->eject({});
                qDebug() << "unmountDisk " << diskId;
                if (diskDev->lastError().isValid()) {
                    qWarning() << diskDev->lastError().name() << diskId;
                    NotifyMsg(tr("Disk is busy, cannot eject now"));
                }
            }
        }
    });
}

void DiskControlWidget::NotifyMsg(QString msg)
{
    DDBusSender()
    .service("org.freedesktop.Notifications")
    .path("/org/freedesktop/Notifications")
    .interface("org.freedesktop.Notifications")
    .method(QString("Notify"))
    .arg(tr("dde-file-manager"))
    .arg(static_cast<uint>(0))
    .arg(QString("media-eject"))
    .arg(msg)
    .arg(QString())
    .arg(QStringList())
    .arg(QVariantMap())
    .arg(5000).call();
}
