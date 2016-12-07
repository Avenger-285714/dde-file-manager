#ifndef COMPUTERDESKTOPFILEINFO_H
#define COMPUTERDESKTOPFILEINFO_H

#include "desktopfileinfo.h"
class ComputerDesktopFileInfo : public DesktopFileInfo
{
public:
    ComputerDesktopFileInfo(const DUrl& fileUrl = ComputerDesktopFileInfo::computerDesktopFileUrl());

    ~ComputerDesktopFileInfo();

    bool isCanRename() const Q_DECL_OVERRIDE;
    bool isCanShare() const Q_DECL_OVERRIDE;
    bool isWritable() const Q_DECL_OVERRIDE;

    QVector<MenuAction> menuActionList(MenuType type = SingleFile) const;
    QList<QIcon> additionalIcon() const;

    static DUrl computerDesktopFileUrl();

};

#endif // COMPUTERDESKTOPFILEINFO_H
