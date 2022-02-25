/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lixiang<lixianga@uniontech.com>
 *
 * Maintainer: lixiang<lixianga@uniontech.com>
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

#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include "preview_plugin_global.h"

#include <danchors.h>

#include <QLabel>

#include <player_widget.h>
#include <player_engine.h>

PREVIEW_BEGIN_NAMESPACE
class VideoPreview;
class VideoWidget : public dmr::PlayerWidget
{
public:
    explicit VideoWidget(VideoPreview *preview);

    QSize sizeHint() const override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    VideoPreview *p;
    QLabel *title;
};
PREVIEW_END_NAMESPACE
#endif   // PLAYERWIDGET_H
