/*
* Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
*
* Author: Liu Zhangjian<liuzhangjian@uniontech.com>
*
* Maintainer: Liu Zhangjian<liuzhangjian@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "dfmvaultremovebypasswordview.h"
#include "interfaceactivevault.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <DToolTip>
#include <DPasswordEdit>
#include <DFloatingWidget>
#include <QTimer>

DFMVaultRemoveByPasswordView::DFMVaultRemoveByPasswordView(QWidget *parent)
    : QWidget (parent)
{
    //密码输入框
    m_pwdEdit = new DPasswordEdit(this);
    m_pwdEdit->lineEdit()->setPlaceholderText(tr("Verify your password"));

    // 提示按钮
    m_tipsBtn = new QPushButton(this);
    m_tipsBtn->setIcon(QIcon(":/icons/images/icons/light_32px.svg"));
    m_tipsBtn->setFixedSize(40, 36);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_pwdEdit);
    layout->addWidget(m_tipsBtn);
    layout->setContentsMargins(0, 15, 0, 0);
    this->setLayout(layout);

    connect(m_pwdEdit->lineEdit(), &QLineEdit::textChanged, this, &DFMVaultRemoveByPasswordView::onPasswordChanged);
    connect(m_tipsBtn, &QPushButton::clicked, [this]{
        QString strPwdHint("");
        if (InterfaceActiveVault::getPasswordHint(strPwdHint)){
            strPwdHint.insert(0, tr("Password hint:"));
            showToolTip(strPwdHint, 3000, EN_ToolTip::Information);
        }
    });
}

DFMVaultRemoveByPasswordView::~DFMVaultRemoveByPasswordView()
{

}

QString DFMVaultRemoveByPasswordView::getPassword()
{
    return m_pwdEdit->text();
}

void DFMVaultRemoveByPasswordView::clear()
{
    // 重置状态
    m_pwdEdit->clear();
    QLineEdit edit;
    QPalette palette = edit.palette();
    m_pwdEdit->lineEdit()->setPalette(palette);
    m_pwdEdit->setEchoMode(QLineEdit::Password);
}

void DFMVaultRemoveByPasswordView::showAlertMessage(const QString &text, int duration)
{
    m_pwdEdit->lineEdit()->setStyleSheet("background-color:rgba(241, 57, 50, 0.15)");
    m_pwdEdit->showAlertMessage(text, duration);
}

void DFMVaultRemoveByPasswordView::showToolTip(const QString &text, int duration, DFMVaultRemoveByPasswordView::EN_ToolTip enType)
{
    if (!m_tooltip){
        m_tooltip = new DToolTip(text);
        m_tooltip->setObjectName("AlertTooltip");
        m_tooltip->setWordWrap(true);

        m_frame = new DFloatingWidget;
        m_frame->setFramRadius(DStyle::pixelMetric(style(), DStyle::PM_FrameRadius));
        m_frame->setStyleSheet("background-color: rgba(247, 247, 247, 0.6);");
        m_frame->setWidget(m_tooltip);
    }
    if(EN_ToolTip::Warning == enType){
        m_pwdEdit->lineEdit()->setStyleSheet("background-color:rgba(241, 57, 50, 0.15)");
        m_tooltip->setForegroundRole(DPalette::TextWarning);
    }
    else{
        m_tooltip->setForegroundRole(DPalette::TextTitle);
    }

    if(parentWidget()){
        if(parentWidget()->parentWidget()){
            if(parentWidget()->parentWidget()->parentWidget())
            m_frame->setParent(parentWidget()->parentWidget()->parentWidget());
        }
    }

    m_tooltip->setText(text);
    if(m_frame->parent()){
        m_frame->setGeometry(6, 180, 68, 26);
        m_frame->show();
        m_frame->adjustSize();
        m_frame->raise();
    }

    if (duration < 0) {
        return;
    }

    QTimer::singleShot(duration, this, [=] {
        m_frame->close();
    });
}

void DFMVaultRemoveByPasswordView::setTipsButtonVisible(bool visible)
{
    m_tipsBtn->setVisible(visible);
}

void DFMVaultRemoveByPasswordView::onPasswordChanged(const QString &password)
{
    if (!password.isEmpty()){
        QLineEdit edit;
        QPalette palette = edit.palette();
        m_pwdEdit->lineEdit()->setPalette(palette);
    }
}
