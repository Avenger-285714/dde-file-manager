/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "display.h"

#include <QScreen>
#include <QApplication>
#define private public
#include <private/qhighdpiscaling_p.h>

#include <dbus/dbusdisplay.h>
#include <dbus/dbusdock.h>

QScreen *GetPrimaryScreen()
{
    return Display::instance()->primaryScreen();
}

Display::Display(QObject *parent) : QObject(parent)
{
#ifdef DDE_DBUS_DISPLAY
    m_appearance = new DBusAppearance(this);
    m_display = new DBusDisplay(this);
    connect(m_display, &DBusDisplay::PrimaryChanged,this, [ = ]() {
        qDebug()<< "primarChanged emit....  ";
        emit  primaryChanged();
    });
    connect(m_display, &DBusDisplay::DisplayModeChanged,this, [ = ]() {
        qDebug()<< "sigDisplayModeChanged emit....  ";
        emit  sigDisplayModeChanged();
    });

    connect(m_display, &DBusDisplay::PrimaryRectChanged, this, [ = ]() {
        auto primaryName = m_display->primary();
        //if X11
        /*for (auto screen : qApp->screens()) {
            if (screen && screen->name() == primaryName) {
                emit primaryScreenChanged(screen);
                return;
            }
        }*/
        //else

        auto e = QProcessEnvironment::systemEnvironment();
        QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
        QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

        if (XDG_SESSION_TYPE == QLatin1String("wayland") ||
                WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
            emit primaryScreenChanged(primaryScreen());
            emit  primaryChanged();
        }

        else {
            for (auto screen : qApp->screens()) {
                if (screen && screen->name() == primaryName) {
                    emit primaryScreenChanged(screen);
                    return;
                }
            }
        }



        qCritical() << "find primaryScreen:" << primaryName << primaryScreen();
    });
#else
    connect(qApp, &QApplication::primaryScreenChanged,
            this, &Display::primaryScreenChanged);
#endif
}

QRect Display::primaryRect()
{
    QRect t_screenRect = m_display->primaryRect();

//    qreal t_devicePixelRatio = m_appearance->GetScaleFactor();//Display::instance()->primaryScreen()->devicePixelRatio();
    qreal t_devicePixelRatio = getScaleFactor();
    if (!QHighDpiScaling::m_active) {
        t_devicePixelRatio = 1;
    }

    t_screenRect.setSize(t_screenRect.size() / t_devicePixelRatio);


    return t_screenRect;
}

QString Display::primaryName()
{
    return m_display->primary();
}

QScreen *Display::primaryScreen()
{
#ifdef DDE_DBUS_DISPLAY
//    auto primaryName = m_display->primary();

    //if X11
    //auto primaryName = m_display->primary();
    //else

    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
    QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

    if (XDG_SESSION_TYPE == QLatin1String("wayland") ||
            WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {

        static QPair<QString, QScreen *> s_primaryScreen;
        if (s_primaryScreen.first.isEmpty()) {
//        s_primaryScreen.first = primaryName;
            s_primaryScreen.first = "eDP-1";
            s_primaryScreen.second = qApp->screens().first();
        }

        if (m_display->primary() == s_primaryScreen.first) {
            qCritical() << "primaryScreen:" << s_primaryScreen.second;
            return s_primaryScreen.second;
        }

        return qApp->screens().back();
    }

    else {
        auto primaryName = m_display->primary();

        for (auto screen : qApp->screens()) {
            if (screen && screen->name() == primaryName) {
                return screen;
            }
        }

    }

    qCritical() << "primaryScreen:" << qApp->primaryScreen();
    return qApp->primaryScreen();
#else
    return qApp->primaryScreen();
#endif
}

QStringList Display::monitorObjectPaths() const
{
    QStringList ret;
    for(const QDBusObjectPath &path : m_display->monitors())
        ret << path.path();
    return  ret;
}

double Display::getScaleFactor()
{
    return m_appearance->GetScaleFactor().value();
}

DockIns::DockIns(QObject *parent) : QObject(parent)
{
    m_dock = new DBusDock(this);
}
