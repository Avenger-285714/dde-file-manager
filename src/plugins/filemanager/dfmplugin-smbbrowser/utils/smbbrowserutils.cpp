/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     xushitong<xushitong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
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
#include "smbbrowserutils.h"

#include "dfm-base/base/device/devicecontroller.h"
#include "dfm-base/base/schemefactory.h"
#include "dfm-base/utils/dialogmanager.h"
#include "dfm-base/dfm_event_defines.h"

#include <dfm-framework/framework.h>

#include <QUrl>

DPSMBBROWSER_USE_NAMESPACE

QMutex SmbBrowserUtils::mutex;
QMap<QUrl, SmbShareNode> SmbBrowserUtils::shareNodes;

QString SmbBrowserUtils::scheme()
{
    return "smb";
}

QIcon SmbBrowserUtils::icon()
{
    return QIcon::fromTheme("network-server-symbolic");
}

bool SmbBrowserUtils::mountSmb(const quint64 windowId, const QList<QUrl> urls, QString *)
{
    if (urls.count() != 1)
        return false;

    DFMBASE_USE_NAMESPACE
    QString devUrl = urls.first().toString();
    DeviceController::instance()->mountNetworkDevice(devUrl, [windowId](bool ok, DFMMOUNT::DeviceError err, const QString &mpt) {
        if (!ok && err != DFMMOUNT::DeviceError::GIOErrorAlreadyMounted) {
            DialogManagerInstance->showErrorDialogWhenMountDeviceFailed(err);
        } else {
            dpfInstance.eventDispatcher().publish(GlobalEventType::kChangeCurrentUrl, windowId, QUrl::fromLocalFile(mpt));
        }
    });
    return true;
}
