#include "bookmarkmanager.h"
#include "dfileservices.h"

#include "app/define.h"

#include <QJsonObject>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QByteArray>
#include <QDateTime>
#include <QUrl>

#include <stdlib.h>

#include "shutil/fileutils.h"


BookMarkManager::BookMarkManager(QObject *parent)
    : DAbstractFileController(parent)
    , BaseManager()
{
    load();
    fileService->setFileUrlHandler(BOOKMARK_SCHEME, "", this);
}

BookMarkManager::~BookMarkManager()
{

}

void BookMarkManager::load()
{
    //Migration for old config files, and rmove that codes for further
    QString oldFilePath = QString("%1/%2").arg(QDir().homePath(), ".cache/dde-file-manager/bookmark.json");
    if(QFile::exists(oldFilePath)){
        QString oldData = FileUtils::getFileContent(oldFilePath);
        if(!oldData.isEmpty()){
            FileUtils::writeTextFile(cachePath(), oldData);
            QFile(oldFilePath).remove();
        }
    }

    //TODO: check permission and existence of the path
    QString configPath = cachePath();
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Couldn't open bookmark file!";
        return;
    }
    QByteArray data = file.readAll();
    QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
    loadJson(jsonDoc.object());
    file.close();
}

void BookMarkManager::save()
{
    //TODO: check permission and existence of the path
    QString configPath = cachePath();
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Couldn't write bookmark file!";
        return;
    }
    QJsonObject object;
    writeJson(object);
    QJsonDocument jsonDoc(object);
    file.write(jsonDoc.toJson());
    file.close();
}

QList<BookMarkPointer> BookMarkManager::getBookmarks()
{
    return m_bookmarks;
}

QString BookMarkManager::cachePath()
{
    return getConfigPath("bookmark");
}

void BookMarkManager::loadJson(const QJsonObject &json)
{
    QJsonArray jsonArray = json["Bookmark"].toArray();
    for(int i = 0; i < jsonArray.size(); i++)
    {
        QJsonObject object = jsonArray[i].toObject();
        QString time = object["t"].toString();
        QString name = object["n"].toString();
        QString url = object["u"].toString();
        BookMarkPointer bm(new BookMark(QDateTime::fromString(time), name, DUrl(url)));
        m_bookmarks.append(bm);
    }
}

void BookMarkManager::writeJson(QJsonObject &json)
{
    QJsonArray localArray;
    for(int i = 0; i < m_bookmarks.size(); i++)
    {
        QJsonObject object;
        object["t"] = m_bookmarks.at(i)->getDateTime().toString();
        object["n"] = m_bookmarks.at(i)->getName();
        object["u"] = m_bookmarks.at(i)->getUrl().toString();
        localArray.append(object);
    }
    json["Bookmark"] = localArray;
}

BookMarkPointer BookMarkManager::writeIntoBookmark(int index, const QString &name, const DUrl &url)
{
    BookMarkPointer bookmark(new BookMark(QDateTime::currentDateTime(), name, url));
    m_bookmarks.insert(index, bookmark);
    save();
    return bookmark;
}

void BookMarkManager::removeBookmark(BookMarkPointer bookmark)
{
    m_bookmarks.removeOne(bookmark);
    save();
}

void BookMarkManager::renameBookmark(BookMarkPointer bookmark, const QString &newname)
{
    bookmark->setName(newname);
    save();
}

void BookMarkManager::moveBookmark(int from, int to)
{
    m_bookmarks.move(from, to);
    save();
}

const QList<DAbstractFileInfoPointer> BookMarkManager::getChildren(const DUrl &fileUrl, const QStringList &nameFilters,
                                                                  QDir::Filters filters, QDirIterator::IteratorFlags flags,
                                                                  bool &accepted) const
{
    accepted = true;

    const QString &frav = fileUrl.fragment();

    if(!frav.isEmpty())
    {
        DUrl localUrl = DUrl::fromLocalFile(frav);

        QList<DAbstractFileInfoPointer> list = fileService->getChildren(localUrl, nameFilters, filters, flags);

        return list;
    }

    QList<DAbstractFileInfoPointer> infolist;

    for (int i = 0; i < m_bookmarks.size(); i++)
    {
        BookMarkPointer bm = m_bookmarks.at(i);

        infolist.append(DAbstractFileInfoPointer(/*new BookMark(*bm))*/bm));
    }

    return infolist;
}

const DAbstractFileInfoPointer BookMarkManager::createFileInfo(const DUrl &fileUrl, bool &accepted) const
{
    accepted = true;

    return DAbstractFileInfoPointer(new BookMark(fileUrl));
}
