/*
 * Copyright (C) 2021 ~ 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     yanghao<yanghao@uniontech.com>
 *
 * Maintainer: liuyangming<liuyangming@uniontech.com>
 *             gongheng<gongheng@uniontech.com>
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
#ifndef RECENTPLUGIN_H
#define RECENTPLUGIN_H

#include "dfmplugin_recent_global.h"

#include <dfm-framework/dpf.h>

namespace dfmplugin_recent {
class Recent : public dpf::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.plugin.filemanager" FILE "recent.json")

public:
    virtual void initialize() override;
    virtual bool start() override;

private slots:
    void onRecentDisplayChanged(bool enabled);
    void onWindowOpened(quint64 windId);
    void regRecentCrumbToTitleBar();
    void onAllPluginsStarted();

private:
    void installToSideBar();
    void addFileOperations();
    void addRecentItem();
    void removeRecentItem();

    void followEvents();
    void bindWindows();
};

}
#endif   // RECENTPLUGIN_H
