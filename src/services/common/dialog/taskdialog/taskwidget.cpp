/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     liyigang<liyigang@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#include "taskwidget.h"
#include "dfm-base/interfaces/abstractfileinfo.h"
#include "dfm-base/base/schemefactory.h"
#include "dfm-base/mimetype/mimedatabase.h"
#include "dfm-base/utils/fileutils.h"

#include <DWaterProgress>
#include <DIconButton>
#include <DGuiApplicationHelper>

#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>
#include <QUrl>
#include <QApplication>
#include <QPointer>

DWIDGET_USE_NAMESPACE
DFMBASE_USE_NAMESPACE
DSC_USE_NAMESPACE
static const int kMsgLabelWidth = 350;
static const int kSpeedLabelWidth = 100;
static const char *const kBtnPropertyActionName = "btnType";
static const AbstractJobHandler::JobState kPausedState = AbstractJobHandler::JobState::kPauseState;

ElidedLable::ElidedLable(QWidget *parent)
    : QLabel(parent)
{
}

ElidedLable::~ElidedLable() {}
/*!
 * \brief ElidedLable::setText 设置当前文字的内容
 * \param text
 */
void ElidedLable::setText(const QString &text)
{
    QFontMetrics metrics(font());
    Qt::TextElideMode em = Qt::TextElideMode::ElideMiddle;

    if (!property("TextElideMode").isNull()) {
        int iem = property("TextElideMode").toInt();
        em = static_cast<Qt::TextElideMode>(iem);
    }

    QLabel::setText(metrics.elidedText(text, em, width()));
}

TaskWidget::TaskWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    showConflictButtons(false);
}

TaskWidget::~TaskWidget()
{
    disconnect();
}
/*!
 * \brief TaskWidget::setTaskHandle 连接当前任务处理器的信号，处理当前拷贝信息的显示
 * \param handle 任务处理控制器
 */
void TaskWidget::setTaskHandle(const JobHandlePointer &handle)
{
    if (!handle)
        return;

    JobInfoPointer info { nullptr };
    info = handle->getTaskInfoByNotifyType(AbstractJobHandler::NotifyType::kNotifyStateChangedKey);
    if (!info.isNull()) {
        onHandlerTaskStateChange(info);
    }
    info = handle->getTaskInfoByNotifyType(AbstractJobHandler::NotifyType::kNotifyCurrentTaskKey);
    if (!info.isNull()) {
        onShowTaskInfo(info);
    }
    info = handle->getTaskInfoByNotifyType(AbstractJobHandler::NotifyType::kNotifyProccessChangedKey);
    if (!info.isNull()) {
        onShowTaskProccess(info);
    }
    info = handle->getTaskInfoByNotifyType(AbstractJobHandler::NotifyType::kNotifySpeedUpdatedTaskKey);
    if (!info.isNull()) {
        onShowSpeedUpdatedInfo(info);
    }
    info = handle->getTaskInfoByNotifyType(AbstractJobHandler::NotifyType::kNotifyErrorTaskKey);
    if (!info.isNull()) {
        onShowErrors(info);
    }

    connect(handle.data(), &AbstractJobHandler::proccessChangedNotify, this, &TaskWidget::onShowTaskProccess, Qt::QueuedConnection);
    connect(handle.data(), &AbstractJobHandler::stateChangedNotify, this, &TaskWidget::onHandlerTaskStateChange, Qt::QueuedConnection);
    connect(handle.data(), &AbstractJobHandler::errorNotify, this, &TaskWidget::onShowErrors, Qt::QueuedConnection);
    connect(handle.data(), &AbstractJobHandler::currentTaskNotify, this, &TaskWidget::onShowTaskInfo, Qt::QueuedConnection);
    connect(handle.data(), &AbstractJobHandler::speedUpdatedNotify, this, &TaskWidget::onShowSpeedUpdatedInfo, Qt::QueuedConnection);

    connect(this, &TaskWidget::buttonClicked, handle.data(), &AbstractJobHandler::operateTaskJob, Qt::QueuedConnection);
}
/*!
 * \brief TaskWidget::onButtonClicked 处理所有按钮按下
 */
void TaskWidget::onButtonClicked()
{
    QObject *obj { sender() };
    if (!obj) {
        qWarning() << "the button is null or the button is release!";
        return;
    }
    AbstractJobHandler::SupportActions actions = obj->property(kBtnPropertyActionName).value<AbstractJobHandler::SupportAction>();
    showConflictButtons(actions.testFlag(AbstractJobHandler::SupportAction::kPauseAction));
    actions = chkboxNotAskAgain->isChecked() ? actions | AbstractJobHandler::SupportAction::kRememberAction : actions;
    emit buttonClicked(actions);
}
/*!
 * \brief TaskWidget::onShowErrors 处理和显示错误信息
 * \param info 这个Varint信息map
 * 在我们自己提供的dailog服务中，这个VarintMap必须有存在kSpeedKey（显示任务的右第一个label的显示，类型：QString）、
 * kRemindTimeKey（（显示任务的右第二个label的显示，类型：QString）
 */
void TaskWidget::onShowErrors(const JobInfoPointer JobInfo)
{

    AbstractJobHandler::JobErrorType errorType = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kErrorTypeKey).value<AbstractJobHandler::JobErrorType>();
    QString sourceMsg = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kSourceMsgKey).toString();
    QString targetMsg = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kTargetMsgKey).toString();
    AbstractJobHandler::SupportActions actions = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kActionsKey).value<AbstractJobHandler::SupportActions>();
    lbSrcPath->setText(sourceMsg);
    lbDstPath->setText(targetMsg);
    if (errorType == AbstractJobHandler::JobErrorType::kFileExistsError || errorType == AbstractJobHandler::JobErrorType::kDirectoryExistsError) {
        QUrl source = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kSourceUrlKey).value<QUrl>();
        QUrl target = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kTargetUrlKey).value<QUrl>();
        return onShowConflictInfo(source, target, actions);
    }
    QString errorMsg = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kErrorMsgKey).toString();
    lbErrorMsg->setText(errorMsg);
    lbErrorMsg->setHidden(errorMsg.isEmpty());
    if (!widButton) {
        widButton = createBtnWidget();
        mainLayout->addWidget(widButton);
    }
    widButton->setHidden(false);

    if (!widConfict) {
        widConfict = createConflictWidget();
        rVLayout->addWidget(widConfict);
    }

    if (widConfict)
        widConfict->hide();

    showBtnByAction(actions);
    showConflictButtons(true, false);
}
/*!
 * \brief TaskWidget::onShowConflictInfo 显示冲突界面信息
 * \param source 源文件的url
 * \param target 目标文件的url
 * \param action 支持的操作
 */
void TaskWidget::onShowConflictInfo(const QUrl source, const QUrl target, const AbstractJobHandler::SupportActions action)
{
    if (!widButton) {
        widButton = createBtnWidget();
        mainLayout->addWidget(widButton);
    }
    if (!widConfict) {
        widConfict = createConflictWidget();
        rVLayout->addWidget(widConfict);
    }
    QString error;
    const AbstractFileInfoPointer &originInfo = InfoFactory::create<AbstractFileInfo>(source, true, &error);
    if (!originInfo) {
        lbErrorMsg->setText(QString(tr("create source file %1 Info failed in show conflict Info function!")).arg(source.path()));
        showBtnByAction(AbstractJobHandler::SupportAction::kCancelAction);
        lbErrorMsg->show();
        showConflictButtons(true, false);
        qWarning() << QString("create source file %1 Info failed in show conflict Info function!").arg(source.path());
        return;
    }
    error.clear();
    AbstractFileInfoPointer targetInfo = InfoFactory::create<AbstractFileInfo>(target, true, &error);
    if (!targetInfo) {
        lbErrorMsg->setText(QString(tr("create target file %1 Info failed in show conflict Info function!")).arg(target.path()));
        lbErrorMsg->show();
        showConflictButtons(true, false);
        showBtnByAction(AbstractJobHandler::SupportAction::kCancelAction);
        qWarning() << QString("create source file %1 Info failed in show conflict Info function!").arg(target.path());
        return;
    }

    showBtnByAction(action);

    if (originInfo && targetInfo) {
        QMimeType mimeTypeSrc = MimeDatabase::mimeTypeForUrl(target);
        if (!mimeTypeSrc.isValid()) {
            qWarning() << "get source file mimetype is valid!";
        }
        lbSrcIcon->setPixmap(QIcon::fromTheme(mimeTypeSrc.iconName()).pixmap(48, 48));
        lbSrcModTime->setText(QString(tr("Time modified: %1")).arg(originInfo->lastModified().isValid() ? originInfo->lastModified().toString("yyyy/MM/dd HH:mm:ss") : qApp->translate("MimeTypeDisplayManager", "Unknown")));
        if (originInfo->isDir()) {
            lbSrcTitle->setText(tr("Original folder"));
            QString filecount = originInfo->countChildFile() <= 1 ? QObject::tr("%1 item").arg(originInfo->countChildFile()) : QObject::tr("%1 items").arg(originInfo->countChildFile());
            lbSrcFileSize->setText(QString(tr("Contains: %1")).arg(filecount));
        } else {
            lbSrcTitle->setText(tr("Original file"));
            lbSrcFileSize->setText(QString(tr("Size: %1")).arg(originInfo->sizeFormat()));
        }
        QMimeType mimeTypeDst = MimeDatabase::mimeTypeForUrl(source);
        if (!mimeTypeDst.isValid()) {
            qWarning() << "get source file mimetype is valid!";
        }
        lbDstIcon->setPixmap(QIcon::fromTheme(mimeTypeSrc.iconName()).pixmap(48, 48));
        lbDstModTime->setText(QString(tr("Time modified: %1")).arg(targetInfo->lastModified().isValid() ? targetInfo->lastModified().toString("yyyy/MM/dd HH:mm:ss") : qApp->translate("MimeTypeDisplayManager", "Unknown")));

        if (targetInfo->isDir()) {
            lbDstTitle->setText(tr("Target folder"));
            QString filecount = targetInfo->countChildFile() <= 1 ? QObject::tr("%1 item").arg(targetInfo->countChildFile()) : QObject::tr("%1 items").arg(targetInfo->countChildFile());
            lbDstFileSize->setText(QString(tr("Contains: %1")).arg(filecount));
            lbDstFileSize->setText(QString(tr("Contains: %1")).arg(QObject::tr("%1 item")));
        } else {
            lbDstTitle->setText(tr("Target file"));
            lbDstFileSize->setText(QString(tr("Size: %1")).arg(targetInfo->sizeFormat()));
        }

        widConfict->show();
        widButton->show();
        btnCoexist->setHidden(false);
        showConflictButtons();
    }
}
/*!
 * \brief TaskWidget::onHandlerTaskStateChange 处理和显示当前拷贝任务的状态变化
 * \param info 这个Varint信息map
 * 在我们自己提供的dailog服务中，这个VarintMap必须存在kJobStateKey（当前任务执行的状态，类型：JobState）用来展示暂停和开始按钮状态
 */
void TaskWidget::onHandlerTaskStateChange(const JobInfoPointer JobInfo)
{
    AbstractJobHandler::JobState state = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kSourceMsgKey).value<AbstractJobHandler::JobState>();
    bool isCurPaused = kPausedState == state;
    if (isCurPaused == isPauseState) {
        return;
    }
    if (state == kPausedState) {
        isPauseState = true;
        btnPause->setIcon(QIcon::fromTheme("dfm_task_start"));
        progress->stop();
    } else {
        isPauseState = false;
        btnPause->setIcon(QIcon::fromTheme("dfm_task_pause"));
        progress->start();
    }
}
/*!
 * \brief TaskWidget::onShowTaskInfo 显示当前任务的任务信息
 * \param info 这个Varint信息map
 * 在我们自己提供的dailog服务中，这个VarintMap必须有存在kSourceMsgKey（显示任务的左第一个label的显示，类型：QString）
 * 和kTargetMsgKey显示任务的左第二个label的显示，类型：QString）
 */
void TaskWidget::onShowTaskInfo(const JobInfoPointer JobInfo)
{
    QString source = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kSourceMsgKey).toString();
    QString target = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kTargetMsgKey).toString();
    lbSrcPath->setText(source);
    lbDstPath->setText(target);
    if (lbErrorMsg->isVisible()) {
        lbErrorMsg->setText("");
        lbErrorMsg->hide();
    }
    if (widConfict)
        widConfict->hide();
    if (widButton)
        widButton->hide();
}
/*!
 * \brief TaskWidget::showTaskProccess 显示当前任务进度
 * \param info 这个Varint信息map
 * 在我们自己提供的dailog服务中，这个VarintMap必须有kCurrentProccessKey（当前任务执行的进度，类型qint64）和
 * kTotalSizeKey（当前任务文件的总大小，如果统计文件数量没有完成，值为-1，类型qint64）值来做文件进度的展示
 */
void TaskWidget::onShowTaskProccess(const JobInfoPointer JobInfo)
{
    qint64 current = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kCurrentProccessKey).value<qint64>();
    qint64 total = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kTotalSizeKey).value<qint64>();
    qint64 value = total <= 0 ? 1 : current * 100 / total;

    if (value > 100) {
        value = 100;
    }

    if (value > 0 && value == progress->value()) {
        return;
    } else if (value >= 0 && progress->value() == 0) {
        progress->start();
        progress->setValue(static_cast<int>(value));
        return;
    }

    if (value < 0) {
        progress->stop();
    } else {
        progress->setValue(static_cast<int>(value));
        progress->update();
    }
}
/*!
 * \brief sendDataSyncing 数据同步中信号
 * \param info 这个Varint信息map
 * 在我们自己提供的dailog服务中，这个VarintMap必须有存在kSpeedKey（显示任务的右第一个label的显示，类型：QString）、
 * kRemindTimeKey（（显示任务的右第二个label的显示，类型：QString）
 */
void TaskWidget::onShowSpeedUpdatedInfo(const JobInfoPointer JobInfo)
{
    QString speed = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kSpeedKey).toString();
    QString rmTime = JobInfo->value(AbstractJobHandler::NotifyInfoKey::kRemindTimeKey).toString();
    lbSpeed->setHidden(speed.isEmpty());
    lbSpeed->setText(speed);
    lbRmTime->setHidden(rmTime.isEmpty());
    lbRmTime->setText(rmTime);
}

/*!
 * \brief TaskWidget::initUI 初始化当前任务的界面
 */
void TaskWidget::initUI()
{
    mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    progress = new DWaterProgress(this);
    progress->setFixedSize(64, 64);
    progress->setValue(1);   // fix：使一开始就有一个进度显示
    progress->setValue(0);
    QHBoxLayout *normalLayout = new QHBoxLayout;
    normalLayout->setContentsMargins(20, 10, 20, 10);
    normalLayout->addWidget(progress, Qt::AlignLeft);
    normalLayout->addSpacing(20);

    lbSrcPath = new ElidedLable;
    lbSpeed = new QLabel;
    lbDstPath = new ElidedLable;
    lbRmTime = new QLabel;
    lbSrcPath->setFixedWidth(kMsgLabelWidth);
    lbDstPath->setFixedWidth(kMsgLabelWidth);
    lbSpeed->setFixedWidth(kSpeedLabelWidth);
    lbRmTime->setFixedWidth(kSpeedLabelWidth);

    rVLayout = new QVBoxLayout;
    QHBoxLayout *hLayout1 = new QHBoxLayout;
    hLayout1->addWidget(lbSrcPath, Qt::AlignLeft);
    hLayout1->addSpacing(10);
    hLayout1->addWidget(lbSpeed, Qt::AlignRight);
    hLayout1->addStretch();

    QHBoxLayout *hLayout2 = new QHBoxLayout;
    hLayout2->addWidget(lbDstPath, Qt::AlignLeft);
    hLayout2->addSpacing(10);
    hLayout2->addWidget(lbRmTime, Qt::AlignRight);
    hLayout2->addStretch();

    rVLayout->addLayout(hLayout1);
    rVLayout->addLayout(hLayout2);

    lbErrorMsg = new ElidedLable;
    lbErrorMsg->setFixedWidth(kMsgLabelWidth + kSpeedLabelWidth);
    rVLayout->addWidget(lbErrorMsg);

    normalLayout->addLayout(rVLayout, 1);

    btnStop = new DIconButton(this);
    btnStop->setObjectName("TaskWidgetStopButton");
    QVariant variantStop;
    variantStop.setValue<AbstractJobHandler::SupportAction>(AbstractJobHandler::SupportAction::kStopAction);
    btnStop->setProperty(kBtnPropertyActionName, variantStop);
    btnStop->setIcon(QIcon::fromTheme("dfm_task_stop"));
    btnStop->setFixedSize(24, 24);
    btnStop->setIconSize({ 24, 24 });
    btnStop->setFlat(true);
    btnStop->setAttribute(Qt::WA_NoMousePropagation);

    btnPause = new DIconButton(this);
    btnPause->setObjectName("TaskWidgetPauseButton");
    QVariant variantPause;
    variantPause.setValue<AbstractJobHandler::SupportAction>(AbstractJobHandler::SupportAction::kPauseAction);
    btnPause->setProperty(kBtnPropertyActionName, variantPause);
    btnPause->setIcon(QIcon::fromTheme("dfm_task_pause"));
    btnPause->setIconSize({ 24, 24 });
    btnPause->setFixedSize(24, 24);
    btnPause->setFlat(true);

    normalLayout->addStretch();
    normalLayout->addWidget(btnPause, Qt::AlignRight);
    normalLayout->addSpacing(10);
    normalLayout->addWidget(btnStop, Qt::AlignRight);

    mainLayout->addLayout(normalLayout);

    lbErrorMsg->setVisible(false);
    btnPause->setVisible(false);
    btnStop->setVisible(false);

    initConnection();
}
/*!
 * \brief TaskWidget::initConnection 初始化信号连接
 */
void TaskWidget::initConnection()
{
    connect(btnPause, &QPushButton::clicked, this, &TaskWidget::onButtonClicked);
    connect(btnStop, &QPushButton::clicked, this, &TaskWidget::onButtonClicked);
}
/*!
 * \brief TaskWidget::createConflictWidget 创建任务显示错误信息的widget
 * \return
 */
QWidget *TaskWidget::createConflictWidget()
{
    QWidget *conflictWidget = new QWidget;
    QPalette labelPalette = palette();
    QColor text_color = labelPalette.text().color();
    labelPalette.setColor(QPalette::Text, text_color);

    lbSrcIcon = new QLabel();
    lbSrcIcon->setFixedSize(48, 48);
    lbSrcIcon->setScaledContents(true);

    lbSrcTitle = new ElidedLable();
    lbSrcModTime = new ElidedLable();
    lbSrcModTime->setPalette(labelPalette);

    lbSrcFileSize = new ElidedLable();
    lbSrcFileSize->setFixedWidth(kSpeedLabelWidth);
    lbSrcFileSize->setPalette(labelPalette);

    lbDstIcon = new QLabel();
    lbDstIcon->setFixedSize(48, 48);
    lbDstIcon->setScaledContents(true);

    lbDstTitle = new ElidedLable();
    lbDstModTime = new ElidedLable();
    lbDstModTime->setPalette(labelPalette);

    lbDstFileSize = new ElidedLable();
    lbDstFileSize->setFixedWidth(kSpeedLabelWidth);
    lbDstFileSize->setPalette(labelPalette);

    QGridLayout *conflictMainLayout = new QGridLayout();

    conflictMainLayout->addWidget(lbSrcIcon, 0, 0, 2, 1, Qt::AlignVCenter);
    conflictMainLayout->addWidget(lbSrcTitle, 0, 1, 1, 2, Qt::AlignVCenter);
    conflictMainLayout->addWidget(lbSrcModTime, 1, 1, Qt::AlignVCenter);
    conflictMainLayout->addWidget(lbSrcFileSize, 1, 2, Qt::AlignVCenter);

    conflictMainLayout->addWidget(lbDstIcon, 2, 0, 2, 1, Qt::AlignVCenter);
    conflictMainLayout->addWidget(lbDstTitle, 2, 1, 1, 2, Qt::AlignVCenter);
    conflictMainLayout->addWidget(lbDstModTime, 3, 1, Qt::AlignVCenter);
    conflictMainLayout->addWidget(lbDstFileSize, 3, 2, Qt::AlignVCenter);

    conflictMainLayout->setHorizontalSpacing(4);
    conflictMainLayout->setVerticalSpacing(4);
    conflictMainLayout->setContentsMargins(0, 0, 0, 0);

    conflictMainLayout->setColumnMinimumWidth(1, kMsgLabelWidth - 100);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addLayout(conflictMainLayout);
    hLayout->addStretch();
    conflictWidget->setLayout(hLayout);
    return conflictWidget;
}
/*!
 * \brief TaskWidget::createBtnWidget 创建错误信息显示的按钮
 * \return
 */
QWidget *TaskWidget::createBtnWidget()
{
    QWidget *buttonWidget = new QWidget;
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(12);

    QVariant variantCoexit;
    variantCoexit.setValue<AbstractJobHandler::SupportAction>(AbstractJobHandler::SupportAction::kCoexistAction);
    btnCoexist = new QPushButton(TaskWidget::tr("Keep both", "button"));
    btnCoexist->setProperty(kBtnPropertyActionName, variantCoexit);

    btnSkip = new QPushButton(TaskWidget::tr("Skip", "button"));
    QVariant variantSkip;
    variantSkip.setValue<AbstractJobHandler::SupportAction>(AbstractJobHandler::SupportAction::kSkipAction);
    btnSkip->setProperty(kBtnPropertyActionName, variantSkip);

    btnReplace = new QPushButton(TaskWidget::tr("Replace", "button"));
    QVariant variantReplace;
    variantReplace.setValue<AbstractJobHandler::SupportAction>(AbstractJobHandler::SupportAction::kReplaceAction);
    btnReplace->setProperty(kBtnPropertyActionName, variantReplace);

    btnSkip->setFocusPolicy(Qt::NoFocus);
    btnReplace->setFocusPolicy(Qt::NoFocus);

    btnCoexist->setCheckable(true);
    btnCoexist->setChecked(true);

    btnSkip->setFixedWidth(80);
    btnReplace->setFixedWidth(80);
    btnCoexist->setFixedWidth(160);

    buttonLayout->addStretch(1);
    buttonLayout->addWidget(btnSkip);
    buttonLayout->addWidget(btnReplace);
    buttonLayout->addWidget(btnCoexist);

    buttonLayout->setContentsMargins(0, 0, 0, 0);

    chkboxNotAskAgain = new QCheckBox(TaskWidget::tr("Do not ask again"));
    QVBoxLayout *btnMainLayout = new QVBoxLayout;
    btnMainLayout->addSpacing(0);
    btnMainLayout->addWidget(chkboxNotAskAgain);
    btnMainLayout->addSpacing(0);
    btnMainLayout->addLayout(buttonLayout);
    buttonWidget->setLayout(btnMainLayout);

    connect(btnSkip, &QPushButton::clicked, this, &TaskWidget::onButtonClicked);
    connect(btnReplace, &QPushButton::clicked, this, &TaskWidget::onButtonClicked);
    connect(btnCoexist, &QPushButton::clicked, this, &TaskWidget::onButtonClicked);

    return buttonWidget;
}
/*!
 * \brief TaskWidget::showBtnByAction 根据不同的操作显示不同的按钮
 * \param action
 */
void TaskWidget::showBtnByAction(const AbstractJobHandler::SupportActions &actions)
{
    btnSkip->setHidden(!actions.testFlag(AbstractJobHandler::SupportAction::kSkipAction));
    btnCoexist->setHidden(!actions.testFlag(AbstractJobHandler::SupportAction::kCoexistAction));
    QString btnTxt;
    QVariant variantReplace;
    if (actions.testFlag(AbstractJobHandler::SupportAction::kRetryAction)) {
        btnReplace->setText(tr("Retry", "button"));
        variantReplace.setValue<AbstractJobHandler::SupportAction>(AbstractJobHandler::SupportAction::kRetryAction);
    } else if (actions.testFlag(AbstractJobHandler::SupportAction::kReplaceAction)) {
        btnReplace->setText(tr("RePlace", "button"));
        variantReplace.setValue<AbstractJobHandler::SupportAction>(AbstractJobHandler::SupportAction::kReplaceAction);
    } else if (actions.testFlag(AbstractJobHandler::SupportAction::kMergeAction)) {
        btnReplace->setText(tr("Merge", "button"));
        variantReplace.setValue<AbstractJobHandler::SupportAction>(AbstractJobHandler::SupportAction::kMergeAction);
    } else {
        btnReplace->setHidden(true);
        return;
    }

    btnReplace->setHidden(false);
    btnReplace->setProperty(kBtnPropertyActionName, variantReplace);
}
/*!
 * \brief TaskWidget::showConflictButtons 显示或者隐藏错误信息的界面
 * \param showBtns 是否显示按钮
 * \param showConflict 是否显示错误信息
 */
void TaskWidget::showConflictButtons(bool showBtns, bool showConflict)
{
    if (!widConfict) {
        return;
    }

    int h = 110;
    if (showBtns) {
        h += widButton->sizeHint().height();
        if (showConflict) {
            h += widConfict->sizeHint().height();
        }
    }

    setFixedHeight(h);
    emit heightChanged();
}

void TaskWidget::onMouseHover(const bool hover)
{
    btnPause->setVisible(hover);
    btnStop->setVisible(hover);

    lbSpeed->setHidden(hover);
    lbRmTime->setHidden(hover);

    update(rect());
}

void TaskWidget::enterEvent(QEvent *event)
{
    onMouseHover(true);

    return QWidget::enterEvent(event);
}

void TaskWidget::leaveEvent(QEvent *event)
{
    onMouseHover(false);

    return QWidget::enterEvent(event);
}

void TaskWidget::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    if (opt.state & QStyle::State_MouseOver) {
        int radius = 8;
        QRectF bgRect;
        bgRect.setSize(size());
        QPainterPath path;
        path.addRoundedRect(bgRect, radius, radius);
        QColor bgColor;
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
            bgColor = QColor(0, 0, 0, 13);   //rgba(0,0,0,13); border-radius: 8px
        } else {
            bgColor = QColor(255, 255, 255, 13);   //rgba(255,255,255,13); border-radius: 8px
        }
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillPath(path, bgColor);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.restore();
    } else if (property("drawSeparator").toBool()) {
        QPoint p1, p2;
        p1 = opt.rect.topLeft();
        p2 = opt.rect.topRight();
        QPen oldPen = painter.pen();
        painter.setPen(QPen(opt.palette.brush(foregroundRole()), 1));
        painter.drawLine(p1, p2);
        painter.setPen(oldPen);
    }

    QWidget::paintEvent(event);
}
