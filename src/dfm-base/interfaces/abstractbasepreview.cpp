#include "abstractbasepreview.h"

DFMBASE_USE_NAMESPACE
AbstractBasePreview::AbstractBasePreview(QObject *parent)
    : QObject(parent)
{
}

void AbstractBasePreview::initialize(QWidget *window, QWidget *statusBar)
{
    Q_UNUSED(window)
    Q_UNUSED(statusBar)
}

QWidget *AbstractBasePreview::statusBarWidget() const
{
    return nullptr;
}

Qt::Alignment AbstractBasePreview::statusBarWidgetAlignment() const
{
    return Qt::AlignCenter;
}

QString AbstractBasePreview::title() const
{
    return QString();
}

bool AbstractBasePreview::showStatusBarSeparator() const
{
    return false;
}

void AbstractBasePreview::play()
{
}

void AbstractBasePreview::pause()
{
}

void AbstractBasePreview::stop()
{
}
