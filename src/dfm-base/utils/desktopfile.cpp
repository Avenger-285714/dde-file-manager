/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: liyigang<liyigang@uniontech.com>
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

#include "desktopfile.h"
#include "properties.h"

#include <QFile>
#include <QSettings>
#include <QDebug>

using namespace dfmbase;

DesktopFile::DesktopFile(const QString &fileName)
    : fileName(fileName)
{
    // File validity
    if (fileName.isEmpty() || !QFile::exists(fileName)) {
        return;
    }

    QSettings settings(fileName, QSettings::IniFormat);
    settings.beginGroup("Desktop Entry");
    // Loads .desktop file (read from 'Desktop Entry' group)
    Properties desktop(fileName, "Desktop Entry");

    if (desktop.contains("X-Deepin-AppID")) {
        deepinId = desktop.value("X-Deepin-AppID", settings.value("X-Deepin-AppID")).toString();
    }

    if (desktop.contains("X-Deepin-Vendor")) {
        deepinVendor = desktop.value("X-Deepin-Vendor", settings.value("X-Deepin-Vendor")).toString();
    }

    if (desktop.contains("NoDisplay")) {
        noDisplay = desktop.value("NoDisplay", settings.value("NoDisplay").toBool()).toBool();
    }
    if (desktop.contains("Hidden")) {
        hidden = desktop.value("Hidden", settings.value("Hidden").toBool()).toBool();
    }

    //由于获取的系统语言简写与.desktop的语言简写存在不对应关系，经决定先采用获取的系统值匹配
    //若没匹配到则采用系统值"_"左侧的字符串进行匹配，均为匹配到，才走原未匹配流程
    auto getValueFromSys = [&desktop, &settings](const QString &type, const QString &sysName) -> QString {
        const QString key = QString("%0[%1]").arg(type).arg(sysName);
        return desktop.value(key, settings.value(key)).toString();
    };

    auto getNameByType = [&desktop, &settings, &getValueFromSys](const QString &type) -> QString {
        QString tempSysName = QLocale::system().name();
        QString targetName = getValueFromSys(type, tempSysName);
        if (targetName.isEmpty()) {
            auto strSize = tempSysName.trimmed().split("_");
            if (!strSize.isEmpty()) {
                tempSysName = strSize.first();
                targetName = getValueFromSys(type, tempSysName);
            }

            if (targetName.isEmpty())
                targetName = desktop.value(type, settings.value(type)).toString();
        }

        return targetName;
    };
    localName = getNameByType("Name");
    genericName = getNameByType("GenericName");

    exec = desktop.value("Exec", settings.value("Exec")).toString();
    icon = desktop.value("Icon", settings.value("Icon")).toString();
    type = desktop.value("Type", settings.value("Type", "Application")).toString();
    categories = desktop.value("Categories", settings.value("Categories").toString()).toString().remove(" ").split(";");

    QString mimeTypeTemp = desktop.value("MimeType", settings.value("MimeType").toString()).toString().remove(" ");

    if (!mimeTypeTemp.isEmpty())
        mimeType = mimeTypeTemp.split(";");
    // Fix categories
    if (categories.first().compare("") == 0) {
        categories.removeFirst();
    }
}
//---------------------------------------------------------------------------

QString DesktopFile::desktopFileName() const
{
    return fileName;
}
//---------------------------------------------------------------------------

QString DesktopFile::desktopPureFileName() const
{
    return fileName.split("/").last().remove(".desktop");
}
//---------------------------------------------------------------------------

QString DesktopFile::desktopName() const
{
    return name;
}

QString DesktopFile::desktopLocalName() const
{
    return localName;
}

QString DesktopFile::desktopDisplayName() const
{
    if (deepinVendor == QStringLiteral("deepin") && !genericName.isEmpty()) {
        return genericName;
    }
    return localName.isEmpty() ? name : localName;
}
//---------------------------------------------------------------------------

QString DesktopFile::desktopExec() const
{
    return exec;
}
//---------------------------------------------------------------------------

QString DesktopFile::desktopIcon() const
{
    return icon;
}
//---------------------------------------------------------------------------

QString DesktopFile::desktopType() const
{
    return type;
}

QString DesktopFile::desktopDeepinId() const
{
    return deepinId;
}

QString DesktopFile::desktopDeepinVendor() const
{
    return deepinVendor;
}

bool DesktopFile::isNoShow() const
{
    return noDisplay || hidden;
}

//---------------------------------------------------------------------------

QStringList DesktopFile::desktopCategories() const
{
    return categories;
}
//---------------------------------------------------------------------------

QStringList DesktopFile::desktopMimeType() const
{
    return mimeType;
}
//---------------------------------------------------------------------------
