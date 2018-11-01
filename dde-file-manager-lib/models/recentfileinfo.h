/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#ifndef RECENTFILEINFO_H
#define RECENTFILEINFO_H

#include "dabstractfileinfo.h"

class RecentFileInfo;
typedef QExplicitlySharedDataPointer<RecentFileInfo> RecentPointer;

class RecentFileInfo : public DAbstractFileInfo
{
public:
    explicit RecentFileInfo(const DUrl &url);

    bool makeAbsolute() override;
    bool exists() const override;
    bool isDir() const override;
    bool isReadable() const override;
    bool isWritable() const override;

    QFileDevice::Permissions permissions() const override;
    QVector<MenuAction> menuActionList(MenuType type) const override;
    QSet<MenuAction> disableMenuActionList() const override;
    QList<int> userColumnRoles() const override;
    QVariant userColumnDisplayName(int userColumnRole) const override;
    QVariant userColumnData(int userColumnRole) const override;
    int userColumnWidth(int userColumnRole, const QFontMetrics &fontMetrics) const override;
    QString subtitleForEmptyFloder() const override;
    Qt::ItemFlags fileItemDisableFlags() const override;
    DUrl mimeDataUrl() const override;
    DUrl parentUrl() const override;

    void setReadDateTime(const QString &time);

private:
    QDateTime m_lastReadTime;
    QString m_lastReadTimeStr;
};

#endif // RECENTFILEINFO_H
