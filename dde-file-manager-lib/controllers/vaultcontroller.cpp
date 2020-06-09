/*
 * Copyright (C) 2019 Deepin Technology Co., Ltd.
 *
 * Author:     Gary Wang <wzc782970009@gmail.com>
 *
 * Maintainer: Gary Wang <wangzichong@deepin.com>
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
#include "vaultcontroller.h"
#include "models/vaultfileinfo.h"
#include "dfileservices.h"
#include "dfilewatcher.h"
#include "dfileproxywatcher.h"
#include "vaulthandle.h"
#include "vaulterrorcode.h"
#include "dfmeventdispatcher.h"
#include "dfilestatisticsjob.h"

#include "appcontroller.h"
#include "singleton.h"
#include "dstorageinfo.h"

#include "tag/tagmanager.h"
#include "shutil/fileutils.h"
#include "shutil/dfmfilelistfile.h"
#include "usershare/shareinfo.h"
#include "app/define.h"
#include "usershare/usersharemanager.h"
#include "dialogs/dialogmanager.h"

#include "dfmevent.h"

#include <QProcess>
#include <QStandardPaths>
#include <QStorageInfo>

#include <QDBusInterface>
#include <QDBusPendingCall>
#include <unistd.h>


VaultController * VaultController::cryfs = nullptr;

class VaultControllerPrivate
{
public:
    explicit VaultControllerPrivate(VaultController * CryFs);
    
    CryFsHandle *m_cryFsHandle;
    
private:
    VaultController * q_ptr;
    Q_DECLARE_PUBLIC(VaultController)
};

VaultControllerPrivate::VaultControllerPrivate(VaultController *CryFs):q_ptr(CryFs)
{
    
}

class VaultDirIterator : public DDirIterator
{
public:
    VaultDirIterator(const DUrl &url,
                     const QStringList &nameFilters,
                     QDir::Filters filter,
                     QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags);
    ~VaultDirIterator() override;

    DUrl next() override;
    bool hasNext() const override;
    
    QString fileName() const override;
    DUrl fileUrl() const override;
    const DAbstractFileInfoPointer fileInfo() const override;
    DUrl url() const override;
    
    DFMFileListFile *hiddenFiles = nullptr;
private:
    QDirIterator *iterator;
    bool nextIsCached = false;
    QDir::Filters filters;
};

VaultDirIterator::VaultDirIterator(const DUrl &url, const QStringList &nameFilters,
                                   QDir::Filters filter, QDirIterator::IteratorFlags flags)
    : DDirIterator()
    , filters(filter)
{
    QString path = VaultController::vaultToLocal(url);
    iterator = new QDirIterator(path, nameFilters, filter, flags);

    hiddenFiles = new DFMFileListFile(path);
}

VaultDirIterator::~VaultDirIterator()
{
    if (iterator) {
        delete iterator;
    }

    if (hiddenFiles) {
        delete hiddenFiles;
    }
}

DUrl VaultDirIterator::next()
{
    if (nextIsCached) {
        nextIsCached = false;

        QString path = iterator->filePath();
        return VaultController::localToVault(path);
    }
    return VaultController::localToVault(iterator->next());
}

// 添加过滤，将保险箱中.hidden中记录的文件隐藏
bool VaultDirIterator::hasNext() const
{
    if (nextIsCached) {
        return true;
    }

    bool hasNext = iterator->hasNext();

    if (!hasNext) {
        return false;
    }

    bool showHidden = filters.testFlag(QDir::Hidden);
    DAbstractFileInfoPointer info;

    do {
        const_cast<VaultDirIterator *>(this)->iterator->next();
        QString absoluteFilePath = iterator->fileInfo().absoluteFilePath();
        info = DAbstractFileInfoPointer(new VaultFileInfo(DUrl::fromLocalFile(absoluteFilePath)));

        if (!info->isPrivate() && (showHidden || (!info->isHidden() && !hiddenFiles->contains(info->fileName())))) {
            break;
        }

        info.reset();
    } while (iterator->hasNext());

    // file is exists
    if (info) {
        const_cast<VaultDirIterator *>(this)->nextIsCached = true;

        return true;
    }

    return false;
    //    return iterator->hasNext();
}

QString VaultDirIterator::fileName() const
{
    return iterator->fileName();
}

DUrl VaultDirIterator::fileUrl() const
{
    return VaultController::localToVault(iterator->filePath());
}

const DAbstractFileInfoPointer VaultDirIterator::fileInfo() const
{
    return DFileService::instance()->createFileInfo(nullptr, fileUrl());
}

DUrl VaultDirIterator::url() const
{
    return fileUrl();
}

VaultController::VaultController(QObject *parent)
    : DAbstractFileController(parent),d_ptr(new VaultControllerPrivate(this))
{
    Q_D(VaultController);
    d->m_cryFsHandle = new CryFsHandle;
    connect(this, &VaultController::sigCreateVault, d->m_cryFsHandle, &CryFsHandle::createVault);
    connect(this, &VaultController::sigUnlockVault, d->m_cryFsHandle, &CryFsHandle::unlockVault);
    connect(this, &VaultController::sigLockVault, d->m_cryFsHandle, &CryFsHandle::lockVault);
    
    connect(d->m_cryFsHandle, &CryFsHandle::signalCreateVault, this, &VaultController::signalCreateVault);
    connect(d->m_cryFsHandle, &CryFsHandle::signalUnlockVault, this, &VaultController::signalUnlockVault);
    connect(d->m_cryFsHandle, &CryFsHandle::signalLockVault, this, &VaultController::signalLockVault);
    connect(d->m_cryFsHandle, &CryFsHandle::signalReadError, this, &VaultController::signalReadError);
    connect(d->m_cryFsHandle, &CryFsHandle::signalReadOutput, this, &VaultController::signalReadOutput);

    // Get root dir size.
    m_sizeWorker = new DFileStatisticsJob(this);

    DUrl rootUrl = vaultToLocalUrl(makeVaultUrl());
    m_sizeWorker->start({rootUrl});
    connect(m_sizeWorker, &DFileStatisticsJob::dataNotify, this, &VaultController::updateFolderSizeLabel);
}

VaultController *VaultController::getVaultController()
{
    if(!cryfs)
    {
        DUrl url(DFMVAULT_ROOT);
        url.setScheme(DFMVAULT_SCHEME);
        QList<DAbstractFileController *> vaultObjlist = DFileService::getHandlerTypeByUrl(url);
        if(vaultObjlist.size() > 0)
            cryfs = static_cast<VaultController *>(vaultObjlist.first());
    }
    return cryfs;
}

const DAbstractFileInfoPointer VaultController::createFileInfo(const QSharedPointer<DFMCreateFileInfoEvent> &event) const
{
    DUrl url(makeVaultUrl());

    if(url == event->url())
    {
        return DAbstractFileInfoPointer(new VaultFileInfo(makeVaultUrl(makeVaultLocalPath())));
    }

    return DAbstractFileInfoPointer(new VaultFileInfo(event->url()));
}

const DDirIteratorPointer VaultController::createDirIterator(const QSharedPointer<DFMCreateDiriterator> &event) const
{
    DUrl url = event->url();
    if(event->url() == makeVaultUrl())
    {
        url = makeVaultUrl(makeVaultLocalPath());
    }
    return DDirIteratorPointer(new VaultDirIterator(url, event->nameFilters(), event->filters(), event->flags()));
}

DAbstractFileWatcher *VaultController::createFileWatcher(const QSharedPointer<DFMCreateFileWatcherEvent> &event) const
{
    QString urlPath = event->url().toLocalFile();
    DUrl url = makeVaultUrl(urlPath);
    auto watcher = new DFileProxyWatcher(url,
                                 new DFileWatcher(urlPath),
                                 VaultController::localUrlToVault);

    connect(watcher, &DFileProxyWatcher::fileDeleted, this, &VaultController::refreshTotalSize);
    connect(watcher, &DFileProxyWatcher::subfileCreated, this, &VaultController::refreshTotalSize);
    connect(watcher, &DFileProxyWatcher::fileMoved, this, &VaultController::refreshTotalSize);
    connect(watcher, &DFileProxyWatcher::fileAttributeChanged, this, &VaultController::refreshTotalSize);

    return watcher;
}

bool VaultController::openFile(const QSharedPointer<DFMOpenFileEvent> &event) const
{
    return DFileService::instance()->openFile(event->sender(), vaultToLocalUrl(event->url()));
}

bool VaultController::openFiles(const QSharedPointer<DFMOpenFilesEvent> &event) const
{
    //    return DFileService::instance()->openFiles(event->sender(), vaultToLocalUrls(event->urlList()));
    DUrlList fileUrls = event->urlList();
    DUrlList packUrl;
    QStringList pathList;
    bool result = false;
    
    for(DUrl fileUrl : fileUrls){
        const DAbstractFileInfoPointer pfile = createFileInfo(dMakeEventPointer<DFMCreateFileInfoEvent>(this, fileUrl));
        
        if (pfile->isSymLink()) {
            const DAbstractFileInfoPointer &linkInfo = DFileService::instance()->createFileInfo(this, pfile->symLinkTarget());
            
            if (linkInfo && !linkInfo->exists()) {
                dialogManager->showBreakSymlinkDialog(linkInfo->fileName(), fileUrl);
                continue;
            }
            fileUrl = linkInfo->redirectedFileUrl();
        }
        
        if (FileUtils::isExecutableScript(fileUrl.toLocalFile())) {
            int code = dialogManager->showRunExcutableScriptDialog(fileUrl, event->windowId());
            result = FileUtils::openExcutableScriptFile(fileUrl.toLocalFile(), code) || result;
            continue;
        }
        
        if (FileUtils::isFileRunnable(fileUrl.toLocalFile()) && !pfile->isDesktopFile()) {
            int code = dialogManager->showRunExcutableFileDialog(fileUrl, event->windowId());
            result = FileUtils::openExcutableFile(fileUrl.toLocalFile(), code) || result;
            continue;
        }
        
        if (FileUtils::shouldAskUserToAddExecutableFlag(fileUrl.toLocalFile()) && !pfile->isDesktopFile()) {
            int code = dialogManager->showAskIfAddExcutableFlagAndRunDialog(fileUrl, event->windowId());
            result = FileUtils::addExecutableFlagAndExecuse(fileUrl.toLocalFile(), code) || result;
            continue;
        }
        
        packUrl << fileUrl;
        QString url = vaultToLocal(fileUrl);
        if (FileUtils::isFileWindowsUrlShortcut(url)) {
            url = FileUtils::getInternetShortcutUrl(url);
        }
        pathList << url;
    }
    
    if (!pathList.empty()) {
        result = FileUtils::openFiles(pathList);
        if (!result) {
            for (const DUrl &fileUrl : packUrl){
                AppController::instance()->actionOpenWithCustom(dMakeEventPointer<DFMOpenFileEvent>(event->sender(),fileUrl)); // requestShowOpenWithDialog
            }
        }
    }
    
    return result;
}

bool VaultController::deleteFiles(const QSharedPointer<DFMDeleteEvent> &event) const
{
    DUrlList urlList = vaultToLocalUrls(event->urlList());        
    DFileService::instance()->deleteFiles(event->sender(), urlList);

    return true;
}

DUrlList VaultController::moveToTrash(const QSharedPointer<DFMMoveToTrashEvent> &event) const
{
    DUrlList urlList = vaultToLocalUrls(event->urlList());
    DFileService::instance()->deleteFiles(event->sender(), urlList);

    return urlList;
}

DUrlList VaultController::pasteFile(const QSharedPointer<DFMPasteEvent> &event) const
{
    DUrlList urlList = vaultToLocalUrls(event->urlList());
    DUrl url = vaultToLocalUrl(event->targetUrl());
    DUrlList ulist = DFileService::instance()->pasteFile(event->sender(), event->action(), url, urlList);
    return ulist;
}

bool VaultController::writeFilesToClipboard(const QSharedPointer<DFMWriteUrlsToClipboardEvent> &event) const
{    
    DUrlList urlList = vaultToLocalUrls(event->urlList());
    return DFileService::instance()->writeFilesToClipboard(event->sender(), event->action(), urlList);
}

bool VaultController::renameFile(const QSharedPointer<DFMRenameEvent> &event) const
{
    return DFileService::instance()->renameFile(event->sender(),
                                                vaultToLocalUrl(event->fromUrl()),
                                                vaultToLocalUrl(event->toUrl()));
}

bool VaultController::shareFolder(const QSharedPointer<DFMFileShareEvent> &event) const
{
    ShareInfo info;
    info.setPath(makeVaultLocalPath(event->name()));
    
    info.setShareName(event->name());
    info.setIsGuestOk(event->allowGuest());
    info.setIsWritable(event->isWritable());
    
    bool ret = userShareManager->addUserShare(info);
    
    return ret;
}

bool VaultController::unShareFolder(const QSharedPointer<DFMCancelFileShareEvent> &event) const
{
    QString path = vaultToLocalUrl(event->fileUrl()).path();
    userShareManager->deleteUserShareByPath(path);
    
    return true;
}

bool VaultController::openInTerminal(const QSharedPointer<DFMOpenInTerminalEvent> &event) const
{
    const QString &current_dir = QDir::currentPath();
    
    QDir::setCurrent(vaultToLocalUrl(event->url()).toLocalFile());
    
    bool ok = QProcess::startDetached(FileUtils::defaultTerminalPath());
    
    QDir::setCurrent(current_dir);
    
    return ok;
}

bool VaultController::addToBookmark(const QSharedPointer<DFMAddToBookmarkEvent> &event) const
{
    DUrl destUrl = event->url();
    
    const DAbstractFileInfoPointer &p = fileService->createFileInfo(nullptr, destUrl);
    DUrl bookmarkUrl = DUrl::fromBookMarkFile(destUrl, p->fileDisplayName());
    DStorageInfo info(destUrl.path());
    QString filePath = destUrl.path();
    QString rootPath = info.rootPath();
    if (rootPath != QStringLiteral("/") || rootPath != QStringLiteral("/home")) {
        QString devStr = info.device();
        QString locateUrl;
        int endPos = filePath.indexOf(rootPath);
        if (endPos != -1) {
            endPos += rootPath.length();
            locateUrl = filePath.mid(endPos);
        }
        if (devStr.startsWith(QStringLiteral("/dev/"))) {
            devStr = DUrl::fromDeviceId(info.device()).toString();
        }
        
        QUrlQuery query;
        query.addQueryItem("mount_point", devStr);
        query.addQueryItem("locate_url", locateUrl);
        bookmarkUrl.setQuery(query);
    }
    
    return DFileService::instance()->touchFile(event->sender(), bookmarkUrl);
}

bool VaultController::removeBookmark(const QSharedPointer<DFMRemoveBookmarkEvent> &event) const
{
    return DFileService::instance()->deleteFiles(nullptr, {DUrl::fromBookMarkFile(event->url(), QString())}, false);
}

bool VaultController::createSymlink(const QSharedPointer<DFMCreateSymlinkEvent> &event) const
{
    QString path = vaultToLocalUrl(event->fileUrl()).path();
    QFile file(path);
    
    QUrl url = event->toUrl().toLocalFile();
    
    bool ok = file.link(event->toUrl().toLocalFile());
    
    if (ok) {
        return true;
    }
    
    if (event->force()) {
        // replace symlink, remove if target was existed
        QFileInfo toLink(event->toUrl().toLocalFile());
        if (toLink.isSymLink() || toLink.exists()) {
            QFile::remove(event->toUrl().toLocalFile());
        }
    }
    
    int code = ::symlink(event->fileUrl().toLocalFile().toLocal8Bit().constData(),
                         event->toUrl().toLocalFile().toLocal8Bit().constData());
    if (code == -1) {
        ok = false;
        QString errorString = strerror(errno);
        dialogManager->showFailToCreateSymlinkDialog(errorString);
    } else {
        ok = true;
    }
    
    return ok;
}

bool VaultController::setFileTags(const QSharedPointer<DFMSetFileTagsEvent> &event) const
{
    DUrl url = event->url();
    DUrl durl = vaultToLocalUrl(url);
    QList<QString> taglist = event->tags();
    if (taglist.isEmpty()) {
        const QStringList &tags = TagManager::instance()->getTagsThroughFiles({durl});
        
        return tags.isEmpty() || TagManager::instance()->removeTagsOfFiles(tags, {durl});
    }
    
    return TagManager::instance()->makeFilesTags(taglist, {durl});
}

bool VaultController::removeTagsOfFile(const QSharedPointer<DFMRemoveTagsOfFileEvent> &event) const
{
    DUrl url = event->url();
    DUrl durl = vaultToLocalUrl(url);
    QList<QString> taglist = event->tags();
    return TagManager::instance()->removeTagsOfFiles(taglist, {durl});
}

QList<QString> VaultController::getTagsThroughFiles(const QSharedPointer<DFMGetTagsThroughFilesEvent> &event) const
{
    DUrlList urllist = event->urlList();
    DUrlList tempList = vaultToLocalUrls(urllist);
    return TagManager::instance()->getTagsThroughFiles(tempList);
}

bool VaultController::setPermissions(const QSharedPointer<DFMSetPermissionEvent> &event) const
{
    DUrl url = event->url();
    DUrl durl = vaultToLocalUrl(url);
    return DFileService::instance()->setPermissions(event->sender(), durl, event->permissions());
}

DUrl VaultController::makeVaultUrl(QString path, QString host)
{
    Q_UNUSED(host)
    // blumia: if path is not start with a `/`, QUrl::setPath will destory the whole QUrl
    //         and only leave the path to the QUrl.
    if (!path.startsWith('/')) {
        path = '/' + path;
    }
    
    DUrl newUrl;
    newUrl.setScheme(DFMVAULT_SCHEME);
    newUrl.setPath(path);
    return newUrl;
}

DUrl VaultController::localUrlToVault(const DUrl &vaultUrl)
{
    return VaultController::localToVault(vaultUrl.path());
}

DUrl VaultController::localToVault(QString localPath)
{
    if(isVaultFile(localPath))
    {
        return VaultController::makeVaultUrl(localPath);
    }
    else
    {
        return DUrl();
    }
}

QString VaultController::vaultToLocal(const DUrl &vaultUrl)
{
    if (vaultUrl.scheme() == DFMVAULT_SCHEME) {
        if(vaultUrl == makeVaultUrl("/"))
            return makeVaultLocalPath(vaultUrl.path());
        else {
            return vaultUrl.toLocalFile();
        }
    }

    return vaultUrl.toLocalFile();
}

DUrl VaultController::vaultToLocalUrl(const DUrl &vaultUrl)
{
    if (vaultUrl.scheme() != DFMVAULT_SCHEME) return vaultUrl;
    return DUrl::fromLocalFile(vaultToLocal(vaultUrl));
}

DUrlList VaultController::vaultToLocalUrls(DUrlList vaultUrls)
{
    for (DUrl &url : vaultUrls) {
        url = vaultToLocalUrl(url);
    } 
    return vaultUrls;
}

VaultController::VaultState VaultController::state(QString lockBaseDir)
{
    QString cryfsBinary = QStandardPaths::findExecutable("cryfs");
    if (cryfsBinary.isEmpty()) {
        return NotAvailable;
    }
    
    if(lockBaseDir.isEmpty())
    {
        lockBaseDir = makeVaultLocalPath("cryfs.config", "vault_encrypted");
    }
    else
    {
        if(lockBaseDir.endsWith("/"))
            lockBaseDir += "cryfs.config";
        else
            lockBaseDir += "/cryfs.config";
    }
    if (QFile::exists(lockBaseDir))
    {
        QStorageInfo info(makeVaultLocalPath(""));
        QString temp = info.fileSystemType();
        if (info.isValid() && temp == "fuse.cryfs")
        {
            return Unlocked;
        }
        
        return Encrypted;
    } else {
        return NotExisted;
    }
}

bool VaultController::isRootDirectory(QString path) const
{
    bool bRootDir = false;
    QString localFilePath = makeVaultLocalPath();
    if (localFilePath == path || makeVaultUrl().toString() == path) {
        bRootDir = true;
    }
    return bRootDir;
}

QString VaultController::getErrorInfo(int state)
{
    QString strErr("");
    switch (state) {
    case 10:
        strErr = tr("The command line arguments are invalid.");
        break;
    case 11:
        strErr = tr("Couldn't load config file. Probably the password is wrong");
        break;
    case 12:
        strErr = tr("Password cannot be empty");
        break;
    case 13:
        strErr = tr("The file system format is too new for this CryFS version. Please update your CryFS version.");
        break;
    case 14:
        strErr = tr("The file system format is too old for this CryFS version. Run with --allow-filesystem-upgrade to upgrade it.");
        break;
    case 15:
        strErr = tr("The file system uses a different cipher than the one specified on the command line using the --cipher argument.");
        break;
    case 16:
        strErr = tr("Base directory doesn't exist or is inaccessible (i.e. not read or writable or not a directory)");
        break;
    case 17:
        strErr = tr("Mount directory doesn't exist or is inaccessible (i.e. not read or writable or not a directory)");
        break;
    case 18:
        strErr = tr("Base directory can't be a subdirectory of the mount directory");
        break;
    case 19:
        strErr = tr("Something's wrong with the file system.");
        break;
    case 20:
        strErr = tr("The filesystem id in the config file is different to the last time we loaded a filesystem from this basedir. This could mean an attacker replaced the file system with a different one. You can pass the --allow-replaced-filesystem option to allow this.");
        break;
    case 21:
        strErr = tr("The filesystem encryption key differs from the last time we loaded this filesystem. This could mean an attacker replaced the file system with a different one. You can pass the --allow-replaced-filesystem option to allow this.");
        break;
    case 22:
        strErr = tr("The command line options and the file system disagree on whether missing blocks should be treated as integrity violations.");
        break;
    case 23:
        strErr = tr("File system is in single-client mode and can only be used from the client that created it.");
        break;
    case 24:
        strErr = tr("A previous run of the file system detected an integrity violation. Preventing access to make sure the user notices. The file system will be accessible again after the user deletes the integrity state file.");
        break;
    case 25:
        strErr = tr("An integrity violation was detected and the file system unmounted to make sure the user notices.");
        break;
    case 26:
        strErr = tr("Mount directory is not empty.");
        break;
    case 27:
        strErr = tr("Mount directory in use.");
        break;
    case 28:
        strErr = tr("Cryfs not installed.");
        break;
    case 29:
        strErr = tr("Mount directory doesn't exist.");
        break;
    case 30:
        strErr = tr("Mounted directory encrypted.");
        break;
    case 31:
        strErr = tr("No permissions.");
        break;
    case 32:
        strErr = tr("Fusermount does not exist");
        break;
    case 33:
        strErr = tr("An encrypted folder created by Cryfs already exists.");
        break;
    default:
        break;
    }

    return strErr;
}

qint64 VaultController::totalsize() const
{
    return m_totalSize;
}

void VaultController::updateFolderSizeLabel(const qint64 size) noexcept
{
    m_totalSize = size;
}

bool VaultController::isVaultFile(QString path)
{
    bool bVaultFile = false;
    QString rootPath = makeVaultLocalPath();
    if (rootPath.back() == "/") {
        rootPath.chop(1);
    }
    if (path.contains(rootPath)) {
        bVaultFile = true;
    }
    return bVaultFile;
}

QString VaultController::pathToVirtualPath(QString path)
{
    QString nextPath = path;
       int index = nextPath.indexOf("vault_unlocked");
       if (index == -1) {
           // fallback to vault file root dir.
           return VaultController::makeVaultUrl("/").toString();
       }

       index += QString("vault_unlocked").length();

       return VaultController::makeVaultUrl(nextPath.mid(index)).toString();
}

DUrl VaultController::urlToVirtualUrl(QString path)
{
    QString nextPath = path;
       int index = nextPath.indexOf("vault_unlocked");
       if (index == -1) {
           // fallback to vault file root dir.
           return VaultController::makeVaultUrl("/");
       }

       index += QString("vault_unlocked").length();

       return VaultController::makeVaultUrl(nextPath.mid(index));
}

void VaultController::createVault(const DSecureString & passWord, QString lockBaseDir, QString unlockFileDir)
{
    auto createIfNotExist = [](const QString & path){
        if (!QFile::exists(path)) {
            QDir().mkpath(path);
        }
    };
    
    if(lockBaseDir.isEmpty() || unlockFileDir.isEmpty())
    {
        if(state() != NotExisted)
        {
            emit signalCreateVault(static_cast<int>(ErrorCode::EncryptedExist));
            return;
        }
        
        createIfNotExist(makeVaultLocalPath("", "vault_encrypted"));
        createIfNotExist(makeVaultLocalPath("", "vault_unlocked"));
        
        emit sigCreateVault(makeVaultLocalPath("", "vault_encrypted"),
                            makeVaultLocalPath("", "vault_unlocked"),
                            passWord);
    }
    else
    {
        if(state(lockBaseDir) != NotExisted)
        {
            emit signalCreateVault(static_cast<int>(ErrorCode::EncryptedExist));
            return;
        }
        
        createIfNotExist(lockBaseDir);
        createIfNotExist(unlockFileDir);
        emit sigCreateVault(lockBaseDir, unlockFileDir, passWord);
    }
}

void VaultController::unlockVault(const DSecureString &passWord, QString lockBaseDir, QString unlockFileDir)
{
    if(lockBaseDir.isEmpty() || unlockFileDir.isEmpty())
    {
        if(state() != Encrypted)
        {
            emit signalUnlockVault(static_cast<int>(ErrorCode::MountpointNotEmpty));
            return;
        }
        
        emit sigUnlockVault(makeVaultLocalPath("", "vault_encrypted"),
                            makeVaultLocalPath("", "vault_unlocked"),
                            passWord);
    }
    else
    {
        if(state(lockBaseDir) != Encrypted)
        {
            emit signalUnlockVault(static_cast<int>(ErrorCode::MountpointNotEmpty));
            return;
        }
        emit sigUnlockVault(lockBaseDir, unlockFileDir, passWord);
    }
}

void VaultController::lockVault(QString lockBaseDir, QString unlockFileDir)
{
    if(lockBaseDir.isEmpty() || unlockFileDir.isEmpty())
    {
        if(state() != Unlocked)
        {
            emit signalLockVault(static_cast<int>(ErrorCode::MountdirEncrypted));
            return;
        }
        emit sigLockVault(makeVaultLocalPath("", "vault_unlocked"));
    }
    else
    {
        if(state(lockBaseDir) != Unlocked)
        {
            emit signalLockVault(static_cast<int>(ErrorCode::MountdirEncrypted));
            return;
        }
        emit sigLockVault(unlockFileDir);
    }
}

QString VaultController::makeVaultLocalPath(QString path, QString base)
{
    if(base.isEmpty())
    {
        base = "vault_unlocked";
    }
    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
            + QDir::separator() + base + (path.startsWith('/') ? "" : "/") + path;
}

QString VaultController::vaultLockPath()
{
    return makeVaultLocalPath("", "vault_encrypted");
}

QString VaultController::vaultUnlockPath()
{
    return makeVaultLocalPath("", "vault_unlocked");
}

void VaultController::refreshTotalSize()
{
    if (m_sizeWorker->isRunning()) {
        m_sizeWorker->stop();
        m_sizeWorker->wait();
    }
    DUrl url = vaultToLocalUrl(makeVaultUrl());
    m_sizeWorker->start({url});
}
