#ifndef DFILEMENU_H
#define DFILEMENU_H

#include <QMenu>

#include "dfmevent.h"

class DFileMenu : public QMenu
{
    Q_OBJECT

public:
    explicit DFileMenu(QWidget * parent = 0);
    DFMEvent event() const;
    void setEvent(const DFMEvent &event);

    QAction *actionAt(int index) const;
    QAction *actionAt(const QString &text) const;

    QAction *exec();
    using QMenu::exec;

    quint64 eventId() const;

private:
    DFMEvent m_event;
};

#endif // DFILEMENU_H
