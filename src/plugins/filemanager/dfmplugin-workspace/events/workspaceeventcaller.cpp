/*
 * Copyright (C) 2021 ~ 2022 Uniontech Software Technology Co., Ltd.
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
#include "workspaceeventcaller.h"

#include "services/filemanager/workspace/workspace_defines.h"
#include "services/common/propertydialog/property_defines.h"
#include "services/filemanager/detailspace/detailspace_defines.h"
#include "services/common/preview/preview_defines.h"
#include "dfm-base/dfm_event_defines.h"
#include "dfm-base/base/schemefactory.h"
#include "dfm_global_defines.h"

#include <dfm-framework/framework.h>

DFMGLOBAL_USE_NAMESPACE
DSB_FM_USE_NAMESPACE
DPWORKSPACE_USE_NAMESPACE
DFMBASE_USE_NAMESPACE

static DPF_NAMESPACE::EventDispatcherManager *
dispatcher()
{
    return &dpfInstance.eventDispatcher();
}

void WorkspaceEventCaller::sendOpenWindow(const QList<QUrl> &urls)
{
    if (urls.isEmpty()) {
        dispatcher()->publish(GlobalEventType::kOpenNewWindow, QUrl());
    } else {
        for (const QUrl &url : urls)
            dispatcher()->publish(GlobalEventType::kOpenNewWindow, url);
    }
}

void WorkspaceEventCaller::sendChangeCurrentUrl(const quint64 windowId, const QUrl &url)
{
    if (!url.isEmpty())
        dispatcher()->publish(GlobalEventType::kChangeCurrentUrl, windowId, url);
}

void WorkspaceEventCaller::sendTabAdded(const quint64 windowID)
{
    dispatcher()->publish(Workspace::EventType::kTabAdded, windowID);
}

void WorkspaceEventCaller::sendTabChanged(const quint64 windowID, const int index)
{
    dispatcher()->publish(Workspace::EventType::kTabChanged, windowID, index);
}

void WorkspaceEventCaller::sendTabMoved(const quint64 windowID, const int from, const int to)
{
    dispatcher()->publish(Workspace::EventType::kTabMoved, windowID, from, to);
}

void WorkspaceEventCaller::sendTabRemoved(const quint64 windowID, const int index)
{
    dispatcher()->publish(Workspace::EventType::kTabRemoved, windowID, index);
}

void WorkspaceEventCaller::sendShowCustomTopWidget(const quint64 windowID, const QString &scheme, bool visible)
{
    dispatcher()->publish(DSB_FM_NAMESPACE::Workspace::EventType::kShowCustomTopWidget, windowID, scheme, visible);
}

void WorkspaceEventCaller::sendSetSelectDetailFileUrl(const quint64 windowId, const QUrl &url)
{
    dispatcher()->publish(DSB_FM_NAMESPACE::DetailEventType::kSetDetailViewSelectFileUrl, windowId, url);
}

void WorkspaceEventCaller::sendShowPreviewDialog(const quint64 windowId, const QList<QUrl> &urls, const QList<QUrl> currentDirUrls)
{
    dispatcher()->publish(DSC_NAMESPACE::Preview::EventType::kShowPreviewEvent, windowId, urls, currentDirUrls);
}
