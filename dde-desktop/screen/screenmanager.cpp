#include "screenmanager.h"
#include "screenobject.h"
#include "dbus/dbusdisplay.h"
#include <QGuiApplication>

#define SCREENOBJECT(screen) dynamic_cast<ScreenObject*>(screen)
ScreenManager::ScreenManager(QObject *parent)
    : AbstractScreenManager(parent)
{
    m_display = new DBusDisplay(this);
    init();
}

ScreenManager::~ScreenManager()
{

}

void ScreenManager::onScreenAdded(QScreen *screen)
{
    if (m_screens.contains(screen))
        return;

    ScreenObjectPointer psc(new ScreenObject(screen));
    m_screens.insert(screen,psc);
    connectScreen(psc);

    emit sigScreenChanged();
}

void ScreenManager::onScreenRemoved(QScreen *screen)
{
    auto psc = m_screens.take(screen);
    if (psc.get() != nullptr){
        disconnectScreen(psc);
        emit sigScreenChanged();
    }
}

void ScreenManager::onScreenGeometryChanged(const QRect &rect)
{
    ScreenObject *sc = SCREENOBJECT(sender());
    if (sc != nullptr && m_screens.contains(sc->screen())) {
        emit sigScreenGeometryChanged(m_screens.value(sc->screen()), rect);
    }
}

void ScreenManager::onScreenAvailableGeometryChanged(const QRect &rect)
{
    ScreenObject *sc = SCREENOBJECT(sender());
    if (sc != nullptr && m_screens.contains(sc->screen())) {
        emit sigScreenAvailableGeometryChanged(m_screens.value(sc->screen()), rect);
    }
}

void ScreenManager::init()
{
    connect(qApp, &QGuiApplication::screenAdded, this, &ScreenManager::onScreenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &ScreenManager::onScreenRemoved);
    connect(qApp, &QGuiApplication::primaryScreenChanged, this, &AbstractScreenManager::sigScreenChanged);
    connect(m_display, &DBusDisplay::DisplayModeChanged, this, &AbstractScreenManager::sigDisplayModeChanged);

    m_screens.clear();
    for (QScreen *sc : qApp->screens()){
        ScreenPointer psc(new ScreenObject(sc));
        m_screens.insert(sc,psc);
        connectScreen(psc);
    }
}

void ScreenManager::connectScreen(ScreenPointer psc)
{
    connect(psc.get(),&AbstractScreen::sigGeometryChanged,this,
            &ScreenManager::onScreenGeometryChanged);
    connect(psc.get(),&AbstractScreen::sigAvailableGeometryChanged,this,
            &ScreenManager::onScreenAvailableGeometryChanged);
}


void ScreenManager::disconnectScreen(ScreenPointer psc)
{
    disconnect(psc.get(),&AbstractScreen::sigGeometryChanged,this,
               &ScreenManager::onScreenGeometryChanged);
    disconnect(psc.get(),&AbstractScreen::sigAvailableGeometryChanged,this,
                &ScreenManager::onScreenAvailableGeometryChanged);
}

ScreenPointer ScreenManager::primaryScreen()
{
    QScreen *primary = qApp->primaryScreen();
    ScreenPointer ret = m_screens.value(primary);
    Q_ASSERT(ret.get() != nullptr);
    return ret;
}

QVector<ScreenPointer> ScreenManager::screens() const
{
    QVector<ScreenPointer> order;
    for (QScreen *sc : qApp->screens()){
        if (m_screens.contains(sc))
            order.append(m_screens.value(sc));
    }
    return order;
}

QVector<ScreenPointer> ScreenManager::logicScreens() const
{
    QVector<ScreenPointer> order;
    auto screens = qApp->screens();

    //调整主屏幕到第一
    QScreen *primary = qApp->primaryScreen();
    screens.removeOne(primary);
    screens.push_front(primary);

    for (QScreen *sc : screens){
        if (m_screens.contains(sc))
            order.append(m_screens.value(sc));
    }
    return order;
}

ScreenPointer ScreenManager::screen(const QString &name) const
{
    ScreenPointer ret;
    for (const ScreenPointer &sp : m_screens.values()) {
        if (sp->name() == name){
            ret = sp;
            break;
        }
    }
    return ret;
}

qreal ScreenManager::devicePixelRatio() const
{
    return qApp->primaryScreen()->devicePixelRatio();
}

AbstractScreenManager::DisplayMode ScreenManager::displayMode() const
{
    AbstractScreenManager::DisplayMode ret = AbstractScreenManager::DisplayMode(m_display->displayMode());
    return ret;
}

void ScreenManager::reset()
{
    if (m_display)
    {
        delete m_display;
        m_display = nullptr;
    }

    m_display = new DBusDisplay(this);
    init();
}
