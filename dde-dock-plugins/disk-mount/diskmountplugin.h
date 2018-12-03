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

#ifndef DISKMOUNTPLUGIN_H
#define DISKMOUNTPLUGIN_H

#include <QLabel>

#include "pluginsiteminterface.h"
#include "diskcontrolwidget.h"
#include "diskpluginitem.h"

class DiskMountPlugin : public QObject, PluginsItemInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterface)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "disk-mount.json")

public:
    explicit DiskMountPlugin(QObject *parent = 0);

    const QString pluginName() const;
    void init(PluginProxyInterface *proxyInter);

    QWidget *itemWidget(const QString &itemKey);
    QWidget *itemTipsWidget(const QString &itemKey);
    QWidget *itemPopupApplet(const QString &itemKey);

    const QString itemContextMenu(const QString &itemKey);
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked);

    int itemSortKey(const QString &itemKey) override;
    void setSortKey(const QString &itemKey, const int order) override;

private:
    void initCompoments();

    void displayModeChanged(const Dock::DisplayMode mode);

private slots:
    void diskCountChanged(const int count);

private:
    bool m_pluginAdded;

    QLabel *m_tipsLabel;
    DiskPluginItem *m_diskPluginItem;
    DiskControlWidget *m_diskControlApplet;
};

#endif // DISKMOUNTPLUGIN_H
