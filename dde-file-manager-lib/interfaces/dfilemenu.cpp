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

#include "dfilemenu.h"
#include "dfmevent.h"
#include <QTimer>

DFileMenu::DFileMenu(QWidget *parent)
    : QMenu(parent)
{

}

const QSharedPointer<DFMMenuActionEvent> DFileMenu::makeEvent(DFMGlobal::MenuAction action) const
{
    DFMMenuActionEvent *event = new DFMMenuActionEvent(m_sender, this, m_currentUrl, m_selectedUrls, action);

    event->setWindowId(m_eventId);

    return QSharedPointer<DFMMenuActionEvent>(event);
}

void DFileMenu::setEventData(const DUrl &currentUrl, const DUrlList &selectedUrls, quint64 eventId, const QObject *sender)
{
    m_currentUrl = currentUrl;
    m_selectedUrls = selectedUrls;
    m_eventId = eventId;
    m_sender = sender;
}

QAction *DFileMenu::actionAt(int index) const
{
    return actions().value(index);
}

QAction *DFileMenu::actionAt(const QString &text) const
{
    for (QAction *action : actions()) {
        if (action->text() == text) {
            return action;
        }
    }

    return Q_NULLPTR;
}

QAction *DFileMenu::exec()
{
    setCanUse(false);
    QTimer::singleShot(200,this,[this]{

        setCanUse(true);
    });
    return QMenu::exec(QCursor::pos());
}

QAction *DFileMenu::exec(const QPoint &pos, QAction *at)
{
    setCanUse(false);
    QTimer::singleShot(200,this,[this]{
        setCanUse(true);
    });
    return QMenu::exec(pos,at);
}

quint64 DFileMenu::eventId() const
{
    return m_eventId;
}

QSet<DFMGlobal::MenuAction> DFileMenu::ignoreMenuActions() const
{
    return m_ignoreMenuActions;
}

void DFileMenu::setIgnoreMenuActions(const QSet<DFMGlobal::MenuAction> &ignoreMenuActions)
{
    m_ignoreMenuActions = ignoreMenuActions;
}

DUrlList DFileMenu::selectedUrls() const
{
    return m_selectedUrls;
}

bool DFileMenu::event(QEvent *event)
{
    return QMenu::event(event);
}

void DFileMenu::keyPressEvent(QKeyEvent *event)
{
    if (!m_canuse) {
        return;
    }

    QMenu::keyPressEvent(event);
}

void DFileMenu::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_canuse) {
        return;
    }

    QMenu::mouseReleaseEvent(event);
}

void DFileMenu::mousePressEvent(QMouseEvent *event)
{
    if (!m_canuse) {
        return;
    }

    QMenu::mousePressEvent(event);
}

void DFileMenu::actionEvent(QActionEvent *event)
{
    if (!m_canuse) {
        return;
    }

    QMenu::actionEvent(event);
}

void DFileMenu::setCanUse(const bool canuse)
{
    m_canuse = canuse;
}
