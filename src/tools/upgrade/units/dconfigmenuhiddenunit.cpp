// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dconfigmenuhiddenunit.h"
#include "dfm-base/base/configs/dconfig/dconfigmanager.h"

#include <QDebug>

using namespace dfm_upgrade;
DFMBASE_USE_NAMESPACE

namespace DConfigKeys {
static constexpr char kDFMMenuHidden[] { "dfm.menu.hidden" };
static constexpr char kFileManagerActionHidden[] { "dfm.menu.action.hidden" };
static constexpr char kFileDialogActionHidden[] { "dfd.menu.action.hidden" };
static constexpr char kDesktopActionHidden[] { "dd.menu.action.hidden" };
}   // namespace DConfigKeys

DConfigMenuHiddenUnit::DConfigMenuHiddenUnit()
{
}

QString DConfigMenuHiddenUnit::name()
{
    return "DConfigMenuHidden";
}

bool DConfigMenuHiddenUnit::initialize(const QMap<QString, QString> &args)
{
    Q_UNUSED(args);
    return true;
}

bool DConfigMenuHiddenUnit::upgrade()
{
    auto upgradeActions = [](QStringList &actions) {
        for (auto &action : actions) {
            const auto &newVal = mappedActions().value(action, action);
            action = newVal.isEmpty() ? action : newVal;   // if no mapped keys, use old version.
            if (newVal.isEmpty())
                qDebug() << "upgrade: no mapped key, keep old value: " << action;
        }
    };

    using namespace DConfigKeys;
    auto fileManagerActionHidden = DConfigManager::instance()->value(kDefaultCfgPath, kFileManagerActionHidden).toStringList();
    auto desktopActionHidden = DConfigManager::instance()->value(kDefaultCfgPath, kDesktopActionHidden).toStringList();
    auto fileDialogActionHidden = DConfigManager::instance()->value(kDefaultCfgPath, kFileDialogActionHidden).toStringList();

    upgradeActions(fileManagerActionHidden);
    upgradeActions(desktopActionHidden);
    upgradeActions(fileDialogActionHidden);

    DConfigManager::instance()->setValue(kDefaultCfgPath, kDesktopActionHidden, desktopActionHidden);
    DConfigManager::instance()->setValue(kDefaultCfgPath, kFileManagerActionHidden, fileManagerActionHidden);
    DConfigManager::instance()->setValue(kDefaultCfgPath, kFileDialogActionHidden, fileDialogActionHidden);

    return true;
}

const QMap<QString, QString> &DConfigMenuHiddenUnit::mappedActions()
{
    static const QMap<QString, QString> mapped {
        { "Compress", "" },   // TODO(xust): this need to be completed. // TODO(liqiang)
        { "Decompress", "" },
        { "DecompressHere", "" },
        { "BookmarkRename", "" },
        { "NewWindow", "" },
        { "ClearRecent", "" },
        { "AutoMerge", "" },

        { "OpenDisk", "computer-open" },
        { "OpenDiskInNewWindow", "computer-open-in-win" },
        { "OpenDiskInNewTab", "computer-open-in-tab" },
        { "Mount", "computer-mount" },
        { "Unmount", "computer-unmount" },
        { "Eject", "computer-eject" },
        { "SafelyRemoveDrive", "computer-safely-remove" },

        { "AutoSort", "auto-arrange" },
        { "SortBy", "sort-by" },
        { "Name", "sort-by-name" },
        { "Size", "sort-by-size" },
        { "Type", "sort-by-type" },

        { "DisplayAs", "display-as" },
        { "IconSize", "icon-size" },

        { "DisplaySettings", "display-settings" },
        { "WallpaperSettings", "wallpaper-settings" },
        { "SetAsWallpaper", "set-as-wallpaper" },

        { "Property", "property" },

        { "Open", "open" },
        { "OpenFileLocation", "open-file-location" },
        { "OpenInNewWindow", "open-in-new-window" },
        { "OpenInNewTab", "open-in-new-tab" },
        { "OpenAsAdmin", "open-as-administrator" },
        { "OpenWith", "open-with" },
        { "OpenWithCustom", "open-with-custom" },
        { "OpenInTerminal", "open-in-terminal" },

        { "Cut", "cut" },
        { "Copy", "copy" },
        { "Paste", "paste" },
        { "Rename", "rename" },
        { "Delete", "delete" },
        { "CompleteDeletion", "delete" },
        { "SelectAll", "select-all" },

        { "AddToBookMark", "add-bookmark" },
        { "BookmarkRemove", "remove-bookmark" },

        { "CreateSymlink", "create-system-link" },
        { "SendToDesktop", "send-to-desktop" },
        { "SendToRemovableDisk", "send-to" },
        { "SendToBluetooth", "share-to-bluetooth" },

        { "NewFolder", "new-folder" },
        { "NewDocument", "new-document" },
        { "NewText", "new-plain-text" },

        { "Restore", "restore" },
        { "RestoreAll", "restore-all" },
        { "ClearTrash", "empty-trash" },
    };

    return mapped;
}