/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *               2019 ~ 2019 Chris Xiong
 *
 * Author:     Chris Xiong<chirs241097@gmail.com>
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

#ifndef DFMROOTFILEINFO_H
#define DFMROOTFILEINFO_H

#include "dfileinfo.h"

#define SUFFIX_USRDIR "userdir"
#define SUFFIX_GVFSMP "gvfsmp"
#define SUFFIX_UDISKS "localdisk"

class DFMRootFileInfoPrivate;

class DFMRootFileInfo : public DAbstractFileInfo
{
public:

    enum ItemType {
        UserDirectory = DAbstractFileInfo::FileType::CustomType + 1,
        GvfsGeneric,
        GvfsSMB,
        GvfsMTP,
        GvfsGPhoto2,
        GvfsFTP,
        GvfsCamera,
        UDisksRoot,
        UDisksData,
        UDisksFixed,
        UDisksRemovable,
        UDisksOptical
    };

    DFMRootFileInfo(const DUrl &url);
    bool exists() const override;

    QString suffix() const override;
    QString fileDisplayName() const override;

    bool canRename() const override;
    bool canShare() const override;
    bool canFetch() const override;
    bool isReadable() const override;
    bool isWritable() const override;
    bool isExecutable() const override;
    bool isHidden() const override;
    bool isRelative() const override;
    bool isAbsolute() const override;
    bool isShared() const override;
    bool isTaged() const override;
    bool isWritableShared() const override;
    bool isAllowGuestShared() const override;

    bool isFile() const override;
    bool isDir() const override;
    FileType fileType() const override;
    int filesCount() const override;
    QString iconName() const override;

    QVector<MenuAction> menuActionList(MenuType type = SingleFile) const override;

    bool canRedirectionFileUrl() const override;
    DUrl redirectedFileUrl() const override;
    bool canDrop() const override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;

    QVariantHash extraProperties() const override;

    void checkCache();
private:
    QScopedPointer<DFMRootFileInfoPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DFMRootFileInfo)
};

#endif // DFMROOTFILEINFO_H
