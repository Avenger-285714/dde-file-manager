/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#include "coreeventscaller.h"

#include "services/filemanager/windows/windowsservice.h"
#include "services/filemanager/workspace/workspace_defines.h"
#include "services/filemanager/sidebar/sidebar_defines.h"

#include "dfm-base/dfm_event_defines.h"

DSB_FM_USE_NAMESPACE
DIALOGCORE_USE_NAMESPACE

void CoreEventsCaller::sendViewMode(QWidget *sender, DFMBASE_NAMESPACE::Global::ViewMode mode)
{
    quint64 id = WindowsService::service()->findWindowId(sender);
    Q_ASSERT(id > 0);

    dpfInstance.eventDispatcher().publish(DFMBASE_NAMESPACE::GlobalEventType::kSwitchViewMode, id, int(mode));
}

void CoreEventsCaller::sendSelectFiles(quint64 windowId, const QList<QUrl> &files)
{
    dpfInstance.eventDispatcher().publish(DSB_FM_NAMESPACE::Workspace::EventType::kSelectFiles, windowId, files);
}

void CoreEventsCaller::setSidebarItemVisible(const QUrl &url, bool visible)
{
    dpfInstance.eventDispatcher().publish(DSB_FM_NAMESPACE::SideBar::EventType::kItemVisibleSetting,
                                          url, visible);
}
