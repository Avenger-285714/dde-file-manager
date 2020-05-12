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

//fixed:CD display size error
#include "views/dfmopticalmediawidget.h"
#include "gvfs/gvfsmountmanager.h"

#include "dblockdevice.h"
#include "ddiskdevice.h"
#include "controllers/dfmrootcontroller.h"
#include "controllers/vaultcontroller.h"
#include "dfmrootfileinfo.h"
#include "app/define.h"
#include "dfileservices.h"
#include "dabstractfileinfo.h"
#include "deviceinfoparser.h"

#include "views/computerview.h"
#include "shutil/fileutils.h"
#include "computermodel.h"


ComputerModel::ComputerModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_diskm(new DDiskManager(this))
{
    m_diskm->setWatchChanges(true);
    par = qobject_cast<ComputerView*>(parent);
    addItem(makeSplitterUrl(tr("My Directories")));
    QList<DAbstractFileInfoPointer> ch = fileService->getChildren(this, DUrl(DFMROOT_ROOT), {}, nullptr);
    bool splt = false;
    m_nitems = 0;
    for (auto chi : ch) {
        if (chi->suffix() != SUFFIX_USRDIR && !splt) {
            addItem(makeSplitterUrl(tr("Disks")));
            splt = true;
        }
        if (splt) {
            auto r = std::upper_bound(m_items.begin() + findItem(makeSplitterUrl(tr("Disks"))) + 1, m_items.end(), chi,
                                      [](const DAbstractFileInfoPointer &a, const ComputerModelItemData &b) {
                                          return DFMRootFileInfo::typeCompare(a, b.fi);
                                      });
            if (r == m_items.end()) {
                addItem(chi->fileUrl());
            } else {
                insertBefore(chi->fileUrl(), r->url);
            }
        } else {
            addItem(chi->fileUrl());
        }
    }
    m_watcher = fileService->createFileWatcher(this, DUrl(DFMROOT_ROOT), this);
    m_watcher->startWatcher();
    connect(m_watcher, &DAbstractFileWatcher::fileDeleted, this, &ComputerModel::removeItem);
    connect(m_watcher, &DAbstractFileWatcher::subfileCreated, this, [this](const DUrl &url) {
            DAbstractFileInfoPointer fi = fileService->createFileInfo(this, url);
            if (!fi->exists()) {
                return;
            }
            auto r = std::upper_bound(m_items.begin() + findItem(makeSplitterUrl(tr("Disks"))) + 1, m_items.end(), fi,
                                      [](const DAbstractFileInfoPointer &a, const ComputerModelItemData &b) {
                                          return DFMRootFileInfo::typeCompare(a, b.fi);
                                      });
            if (r == m_items.end()) {
                addItem(url);
            } else {
                insertBefore(url, r->url);
            }
    });
    connect(m_watcher, &DAbstractFileWatcher::fileAttributeChanged, [this](const DUrl &url) {
        int p;
        for (p = 0; p < m_items.size(); ++p) {
            if (m_items[p].url == url) {
                break;
            }
        }
        if (p >= m_items.size()) {
            return;
        }
        QModelIndex idx = index(p, 0);
        static_cast<DFMRootFileInfo*>(m_items[p].fi.data())->checkCache();
        emit dataChanged(idx, idx, {Qt::ItemDataRole::DisplayRole});
    });

    // 保险柜
    addItem(makeSplitterUrl(QObject::tr("File Vault")));
    addItem(VaultController::makeVaultUrl());

    connect(deviceListener, SIGNAL(volumeAdded(UDiskDeviceInfoPointer)), this, SLOT(onVolumeAdded(UDiskDeviceInfoPointer)));
}

ComputerModel::~ComputerModel()
{
}

QModelIndex ComputerModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row >= rowCount() || row < 0) {
        return QModelIndex();
    }
    return createIndex(row, column, (void*)&m_items[row]);
}

QModelIndex ComputerModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int ComputerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_items.size();
}

int ComputerModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant ComputerModel::data(const QModelIndex &index, int role) const
{
    if (static_cast<ComputerModelItemData*>(index.internalPointer())->cat == ComputerModelItemData::Category::cat_widget) {
        par->view()->setIndexWidget(index, static_cast<ComputerModelItemData*>(index.internalPointer())->widget);
    }

    const ComputerModelItemData *pitmdata = &m_items[index.row()];

    if (role == Qt::DisplayRole) {
        if (index.data(DataRoles::ICategoryRole) == ComputerModelItemData::Category::cat_splitter) {
            return pitmdata->sptext;
        }
        if (pitmdata->fi) {
            return pitmdata->fi->fileDisplayName();
        }
    }

    if (role == Qt::DecorationRole) {
        if (pitmdata->fi) {

            DFMRootFileInfo::ItemType itemType = static_cast<DFMRootFileInfo::ItemType>(pitmdata->fi->fileType());
            if (itemType == DFMRootFileInfo::UDisksOptical) {
                QString udisk = pitmdata->fi->extraProperties()["udisksblk"].toString();
                QStringList strList = udisk.split("/");
                QString device = strList.back();

                bool isInternal = DeviceInfoParser::Instance().isInternalDevice(device);
                if (!isInternal) {
                    return QIcon::fromTheme("media-external");
                }
            }

            return QIcon::fromTheme(pitmdata->fi->iconName());
        }
    }

    if (role == DataRoles::FileSystemRole) {
        QString fs_type = "";
        if (pitmdata->fi) {
            //! 添加文件系统格式数据
            fs_type = pitmdata->fi->extraProperties()["fsType"].toString().toUpper();
        }

        return fs_type;
    }

    if (role == DataRoles::SizeInUseRole) {
        if (pitmdata->fi) {
            //fixed:CD display size error
            if (static_cast<DFMRootFileInfo::ItemType>(pitmdata->fi->fileType()) == DFMRootFileInfo::ItemType::UDisksOptical) {
                //fix: 探测光盘推进,弹出和挂载状态机标识
                if (!GvfsMountManager::g_burnVolumeFlag && !GvfsMountManager::g_burnMountFlag) { //CD/DVD
                    DFMOpticalMediaWidget::g_usedSize = 0;
                    return DFMOpticalMediaWidget::g_usedSize;
                } else {
                    return DFMOpticalMediaWidget::g_usedSize;
                }
            } else {
                if (pitmdata->fi->fileUrl().scheme() == DFMVAULT_SCHEME) {
                    return QString::number(pitmdata->fi->size());
                }
                return pitmdata->fi->extraProperties()["fsUsed"];
            }
        }
    }

    if (role == DataRoles::SizeTotalRole) {
        if (pitmdata->fi) {
            //fixed:CD display size error
            if (static_cast<DFMRootFileInfo::ItemType>(pitmdata->fi->fileType()) == DFMRootFileInfo::ItemType::UDisksOptical) {
                //fix: 探测光盘推进,弹出和挂载状态机标识
                if (!GvfsMountManager::g_burnVolumeFlag && !GvfsMountManager::g_burnMountFlag) { //CD/DVD
                    DFMOpticalMediaWidget::g_totalSize = 0;
                    return DFMOpticalMediaWidget::g_totalSize;
                } else {
                    return DFMOpticalMediaWidget::g_totalSize;
                }
            } else {
                return pitmdata->fi->extraProperties()["fsSize"];
            }
        }
    }

    if (role == DataRoles::ICategoryRole) {
        return m_items.at(index.row()).cat;
    }

    if (role == DataRoles::OpenUrlRole) {
        if (pitmdata->fi) {
            return QVariant::fromValue(pitmdata->fi->redirectedFileUrl());
        }
    }

    if (role == DataRoles::MountOpenUrlRole) {
        if (pitmdata->fi) {
            QSharedPointer<DBlockDevice> blkdev(DDiskManager::createBlockDevice(pitmdata->fi->extraProperties()["udisksblk"].toString()));
            if (pitmdata->fi->suffix() == SUFFIX_UDISKS && blkdev && blkdev->mountPoints().size() == 0) {
                blkdev->mount({});
            }
            return QVariant::fromValue(pitmdata->fi->redirectedFileUrl());
        }
    }

    if (role == DataRoles::ActionVectorRole) {
        if (pitmdata->fi) {
            return QVariant::fromValue(pitmdata->fi->menuActionList());
        }
    }

    if (role == DataRoles::DFMRootUrlRole) {
        if (pitmdata->fi) {
            return QVariant::fromValue(pitmdata->fi->fileUrl());
        }
    }

    if (role == DataRoles::Scheme) {
        if (pitmdata->fi) {
            return QVariant::fromValue(pitmdata->fi->fileUrl().scheme());
        }
    }

    return QVariant();
}

bool ComputerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        ComputerModelItemData *pitmdata = &m_items[index.row()];
        if (!pitmdata->fi->canRename()) {
            return false;
        }
        fileService->renameFile(this, pitmdata->fi->fileUrl(), DUrl(value.toString()));
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
    }
    return false;
}

Qt::ItemFlags ComputerModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags ret = Qt::ItemFlag::ItemNeverHasChildren;
    if (index.data(DataRoles::ICategoryRole) != ComputerModelItemData::Category::cat_splitter) {
        ret |= Qt::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable;
    }

    const ComputerModelItemData *pitmdata = &m_items[index.row()];
    if (pitmdata->fi && pitmdata->fi->canRename()) {
        ret |= Qt::ItemFlag::ItemIsEditable;
    }
    return ret;
}

QModelIndex ComputerModel::findIndex(const DUrl &url) const
{
    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items[row].url == url) {
            return index(row, 0);
        }
    }
    return QModelIndex();
}

int ComputerModel::itemCount() const
{
    return m_nitems;
}

void ComputerModel::addItem(const DUrl &url, QWidget* w)
{
    if (findItem(url) != -1) {
        return;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount() + 1);
    ComputerModelItemData id;
    initItemData(id, url, w);
    m_items.append(id);
    endInsertRows();
    if (url.scheme() != SPLITTER_SCHEME && url.scheme() != WIDGET_SCHEME) {
        Q_EMIT itemCountChanged(++m_nitems);
    }
}

void ComputerModel::insertAfter(const DUrl &url, const DUrl &ref, QWidget *w)
{
    if (findItem(url) != -1) {
        return;
    }

    int p = findItem(ref);
    if (p == -1) {
        return;
    }

    beginInsertRows(QModelIndex(), p + 1, p + 2);
    ComputerModelItemData id;
    initItemData(id, url, w);
    m_items.insert(p + 1, id);
    endInsertRows();
    if (url.scheme() != SPLITTER_SCHEME && url.scheme() != WIDGET_SCHEME) {
        Q_EMIT itemCountChanged(++m_nitems);
    }
}

void ComputerModel::insertBefore(const DUrl &url, const DUrl &ref, QWidget *w)
{
    if (findItem(url) != -1) {
        return;
    }

    int p = findItem(ref);
    if (p == -1) {
        return;
    }

    beginInsertRows(QModelIndex(), p, p + 1);
    ComputerModelItemData id;
    initItemData(id, url, w);
    m_items.insert(p, id);
    endInsertRows();
    if (url.scheme() != SPLITTER_SCHEME && url.scheme() != WIDGET_SCHEME) {
        Q_EMIT itemCountChanged(++m_nitems);
    }
}

void ComputerModel::removeItem(const DUrl &url)
{
    int p = findItem(url);
    if (p == -1) {
        return;
    }

    beginRemoveRows(QModelIndex(), p, p);
    m_items.removeAt(p);
    endRemoveRows();
    if (url.scheme() != SPLITTER_SCHEME && url.scheme() != WIDGET_SCHEME) {
        Q_EMIT itemCountChanged(--m_nitems);
    }
}

// 用于读取新增设备信息
void ComputerModel::onVolumeAdded(UDiskDeviceInfoPointer diskDeivceInfo)
{
    if (diskDeivceInfo->optical()) {

        std::thread thread(
                []()
                {
                    DeviceInfoParser::Instance().refreshDabase();
                }
        );
        thread.detach();
    }
}

void ComputerModel::initItemData(ComputerModelItemData &data, const DUrl &url, QWidget *w)
{
    data.url = url;
    if (url.scheme() == SPLITTER_SCHEME) {
        data.cat = ComputerModelItemData::Category::cat_splitter;
        data.sptext = url.fragment();
    } else if (url.scheme() == WIDGET_SCHEME) {
        data.cat = ComputerModelItemData::Category::cat_widget;
        data.widget = w;
    } else {
        data.fi = fileService->createFileInfo(this, url);
        if (data.fi->suffix() == SUFFIX_USRDIR) {
            data.cat = ComputerModelItemData::Category::cat_user_directory;
        } else {
            data.cat = ComputerModelItemData::Category::cat_internal_storage;
        }
    }
}

int ComputerModel::findItem(const DUrl &url)
{
    int p;
    for (p = 0; p < m_items.size(); ++p) {
        if (m_items[p].url == url) {
            break;
        }
    }
    if (p >= m_items.size()) {
        return -1;
    }
    return p;
}

DUrl ComputerModel::makeSplitterUrl(QString text)
{
    DUrl ret;
    ret.setScheme(SPLITTER_SCHEME);
    ret.setFragment(text);
    return ret;
}
