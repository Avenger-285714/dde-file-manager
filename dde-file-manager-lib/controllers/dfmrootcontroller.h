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

#ifndef DFMROOTCONTROLLER_H
#define DFMROOTCONTROLLER_H

#include "dabstractfilecontroller.h"
#include "dabstractfilewatcher.h"
#include "durl.h"
#include <dgiomount.h>

class DFMRootFileWatcherPrivate;
class DFMRootFileWatcher : public DAbstractFileWatcher
{
    Q_OBJECT

public:
    explicit DFMRootFileWatcher(const DUrl &url, QObject *parent = nullptr);

private:
    Q_DECLARE_PRIVATE(DFMRootFileWatcher)
};

class DFMRootController : public DAbstractFileController
{
    Q_OBJECT
public:
    explicit DFMRootController(QObject *parent = nullptr);
    bool renameFile(const QSharedPointer<DFMRenameEvent> &event) const override;

    const QList<DAbstractFileInfoPointer> getChildren(const QSharedPointer<DFMGetChildrensEvent> &event) const override;

    const DAbstractFileInfoPointer createFileInfo(const QSharedPointer<DFMCreateFileInfoEvent> &event) const override;
    DAbstractFileWatcher *createFileWatcher(const QSharedPointer<DFMCreateFileWatcherEvent> &event) const override;
};

#endif // DFMROOTCONTROLLER_H
