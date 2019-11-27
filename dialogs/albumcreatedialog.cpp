/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#include "application.h"
#include "albumcreatedialog.h"
#include "dbmanager/dbmanager.h"
#include "utils/baseutils.h"
#include <QHBoxLayout>
#include <DLabel>
#include <DLineEdit>
#include <QKeyEvent>
#include <QDebug>
#include <DPushButton>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DSuggestButton>

AlbumCreateDialog::AlbumCreateDialog(DWidget* parent)
    :DDialog(parent)
{
    initUI();
    initConnection();
}

void AlbumCreateDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        this->close();
    }
}

void AlbumCreateDialog::initUI()
{
    setFixedSize(380,180);
    setModal(true);
    setContentsMargins(0, 0, 0, 0);
//图标
    DLabel *logoLable = new DLabel(this);
    QIcon icon = QIcon::fromTheme("deepin-album");
    logoLable->setPixmap(icon.pixmap(QSize(32, 32)));
    logoLable->move(10, 9);
    logoLable->setAlignment(Qt::AlignLeft);
//title
    const QString subStyle =
            utils::base::getFileContent(":/dialogs/qss/resources/qss/inputdialog.qss");
    DLabel *title = new DLabel(this);
    DFontSizeManager::instance()->bind(title, DFontSizeManager::T5, QFont::DemiBold);
    title->setForegroundRole(DPalette::TextTitle);
    title->setText(tr("New Album"));
//    title->setFixedSize(68,25);
    title->setFixedSize(130,25);
    title->setObjectName("DialogTitle");
    title->setAlignment(Qt::AlignHCenter);
    title->move(130,12);
//编辑框
    edit = new DLineEdit(this);
    edit->setObjectName("DialogEdit");
    edit->setText(getNewAlbumName());
    edit->setContextMenuPolicy(Qt::PreventContextMenu);
    edit->setClearButtonEnabled(false);
    edit->setFixedSize(360, 36);
    edit->move(10,78);
    edit->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
//内容widget
    DWidget *contentWidget = new DWidget(this);
    contentWidget->setContentsMargins(0, 0, 0, 0);
    addContent(contentWidget);
//按钮

    m_Cancel = new DPushButton(this);
    m_Cancel->setText(tr("Cancel"));
    m_Cancel->setFixedSize(170,36);
    m_Cancel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    DPushButton *m_line = new DPushButton(this);
    m_line->setFixedSize(3,28);
    m_line->setEnabled(false);
    m_OK = new DSuggestButton(this);
    m_OK->setText(tr("Create"));
    m_OK->setFixedSize(170,36);
    m_OK->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));

    m_Cancel->move(10,134);
    m_line->move(189,138);
    m_OK->move(200,134);
}

void AlbumCreateDialog::initConnection()
{
    connect(this, &AlbumCreateDialog::visibleChanged, this, [=] (bool v) {
        if (! v) return;
        edit->lineEdit()->selectAll();
        edit->lineEdit()->setFocus();
    });
    connect(edit, &DLineEdit::returnPressed, this, [=] {
        const QString album = edit->text().trimmed();
        if (! album.isEmpty()) {
            createAlbum(album);
            this->close();
        }
    });
    connect(m_Cancel,&DPushButton::clicked,this,&AlbumCreateDialog::deleteLater);
    connect(m_OK,&DPushButton::clicked,this,[=]{
        if (edit->text().simplified().length()!= 0)
        {
            createAlbum(edit->text().trimmed());
        }
        else
        {
            createAlbum(tr("Unnamed"));
        }

        this->close();
    });

//    connect(this, &AlbumCreateDialog::closed,
//            this, &AlbumCreateDialog::deleteLater);
//    connect(this, &AlbumCreateDialog::buttonClicked, this, [=] (int id) {
//        if (id == 1) {
//            if (edit->text().simplified().length()!= 0)
//                createAlbum(edit->text().trimmed());
//            else
//                createAlbum(tr("Unnamed"));
//        }

//    });

}

/*!
 * \brief AlbumCreateDialog::getNewAlbumName
 * \return Return a string like "Unnamed3", &etc
 */
const QString AlbumCreateDialog::getNewAlbumName() const
{
    const QString nan = tr("Unnamed");
       int num = 1;
       QString albumName = nan;
       while(DBManager::instance()->isAlbumExistInDB(albumName)) {
           num++;
           albumName = nan + QString::number(num);
       }
       return (const QString)(albumName);
}

const QString AlbumCreateDialog::getCreateAlbumName() const
{
    return m_createAlbumName;
}

void AlbumCreateDialog::createAlbum(const QString &newName)
{
    if (! DBManager::instance()->getAllAlbumNames().contains(newName)) {
        m_createAlbumName = newName;
        DBManager::instance()->insertIntoAlbum(newName, QStringList(" "));
    }
    else {
        m_createAlbumName = getNewAlbumName();
        DBManager::instance()->insertIntoAlbum(getNewAlbumName(), QStringList(" "));
    }

    emit albumAdded();
}
