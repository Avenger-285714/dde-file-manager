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

#include "encryptionpage.h"

#include <DFontSizeManager>
#include <DApplication>
#include <DLabel>
#include <DGuiApplicationHelper>

#include <QBoxLayout>

DWIDGET_USE_NAMESPACE
PREVIEW_USE_NAMESPACE
EncryptionPage::EncryptionPage(QWidget *parent)
    : QWidget(parent)
{
    InitUI();
    InitConnection();
}

EncryptionPage::~EncryptionPage()
{
}

void EncryptionPage::InitUI()
{
    QPixmap m_encrypticon = QIcon::fromTheme("dr_compress_lock").pixmap(128, 128);
    DLabel *pixmaplabel = new DLabel(this);
    pixmaplabel->setPixmap(m_encrypticon);
    DLabel *stringinfolabel = new DLabel(this);

    DFontSizeManager::instance()->bind(stringinfolabel, DFontSizeManager::T5, QFont::DemiBold);
    stringinfolabel->setForegroundRole(DPalette::ToolTipText);
    stringinfolabel->setText(tr("Encrypted file, please enter the password"));

    passwordEdit = new DPasswordEdit(this);
    passwordEdit->setFixedSize(360, 36);
    QLineEdit *edit = passwordEdit->lineEdit();
    edit->setObjectName("passwdEdit");
    edit->setPlaceholderText(tr("Password"));

    nextbutton = new DPushButton(this);
    nextbutton->setObjectName("ensureBtn");
    nextbutton->setFixedSize(360, 36);
    nextbutton->setText(tr("OK", "button"));
    nextbutton->setDisabled(true);

    QVBoxLayout *mainlayout = new QVBoxLayout(this);
    mainlayout->setSpacing(0);
    mainlayout->addStretch();
    mainlayout->addWidget(pixmaplabel, 0, Qt::AlignCenter);

    mainlayout->addSpacing(4);
    mainlayout->addWidget(stringinfolabel, 0, Qt::AlignCenter);

    mainlayout->addSpacing(30);
    mainlayout->addWidget(passwordEdit, 0, Qt::AlignCenter);

    mainlayout->addSpacing(20);
    mainlayout->addWidget(nextbutton, 0, Qt::AlignCenter);

    mainlayout->addStretch();

    setAutoFillBackground(true);

    onUpdateTheme();
    passwordEdit->lineEdit()->setAttribute(Qt::WA_InputMethodEnabled, false);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &EncryptionPage::onUpdateTheme);
}

void EncryptionPage::InitConnection()
{
    connect(passwordEdit, &DPasswordEdit::textChanged, this, &EncryptionPage::onPasswordChanged);
    connect(nextbutton, &DPushButton::clicked, this, &EncryptionPage::nextbuttonClicked);

    connect(qApp, SIGNAL(sigSetPasswdFocus()), this, SLOT(onSetPasswdFocus()));
}

void EncryptionPage::nextbuttonClicked()
{
    emit sigExtractPassword(passwordEdit->text());
}

void EncryptionPage::wrongPassWordSlot()
{
    passwordEdit->clear();
    passwordEdit->setAlert(true);
    passwordEdit->showAlertMessage(tr("Wrong password"));
    passwordEdit->lineEdit()->setFocus(Qt::TabFocusReason);
}

void EncryptionPage::onPasswordChanged()
{
    if (passwordEdit->isAlert()) {
        passwordEdit->setAlert(false);
        passwordEdit->hideAlertMessage();
    }

    if (passwordEdit->text().isEmpty()) {
        nextbutton->setDisabled(true);
    } else {
        nextbutton->setEnabled(true);
    }
}

void EncryptionPage::onSetPasswdFocus()
{
    if (this->isVisible() && passwordEdit)
        passwordEdit->lineEdit()->setFocus(Qt::TabFocusReason);
}

void EncryptionPage::onUpdateTheme()
{
    DPalette plt = Dtk::Gui::DGuiApplicationHelper::instance()->applicationPalette();
    plt.setColor(Dtk::Gui::DPalette::Background, plt.color(Dtk::Gui::DPalette::Base));
    setPalette(plt);
}
