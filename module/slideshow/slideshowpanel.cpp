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
#include "slideshowpanel.h"
#include "slideeffectplayer.h"
#include "application.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "module/view/viewpanel.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QShortcut>
#include <QStyleFactory>

namespace {

const int DELAY_START_INTERVAL = 500;
const int DELAY_HIDE_CURSOR_INTERVAL = 3000;
const QColor DARK_BG_COLOR = QColor(27, 27, 27);
const QColor LIGHT_BG_COLOR = QColor(255, 255, 255);
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";
const int FROM_MAINWINDOW_POPVIEW = 4;      // 从view页面进入幻灯片

}  // namespace

SlideShowPanel::SlideShowPanel(QWidget *parent)
    : ModulePanel(parent)
    , m_hideCursorTid(0)
    , m_startTid(0)
{
    setFocusPolicy(Qt::StrongFocus);
//    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    m_bgColor = DARK_BG_COLOR;
    initeffectPlay();
    initMenu();
    initShortcut();
    initFileSystemMonitor();
    setMouseTracking(true);
//    m_cancelslideshow = new DIconButton(this);
//    m_cancelslideshow->setIcon(QIcon(":/resources/exit_slider.svg"));
//    m_cancelslideshow->setIconSize(QSize(50, 50));
//    m_cancelslideshow->setFixedSize(QSize(50, 50));
//    connect(m_cancelslideshow, &DIconButton::clicked, m_player,
//    [ = ] {/*m_player->stop(); this->showNormal();emit dApp->signalM->hideImageView(); m_cancelslideshow->hide();*/
//        backToLastPanel();
//    });
    connect(dApp->signalM, &SignalManager::startSlideShow,
            this, &SlideShowPanel::startSlideShow);
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(dApp->signalM, &SignalManager::imagesRemovedPar, [ = ](
    const DBImgInfoList & infos) {
        foreach (DBImgInfo info, infos) {
            if (m_vinfo.paths.contains(info.filePath)) {
                m_vinfo.paths.removeOne(info.filePath);
            }
        }

        m_player->setImagePaths(m_vinfo.paths);
    });
    connect(dApp->signalM, &SignalManager::sigESCKeyStopSlide, this, [ = ] {
        if (isVisible())
        {
            backToLastPanel();
        }
    });
//    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this,
//            &SlideShowPanel::onThemeChanged);

    slideshowbottombar = new SlideShowBottomBar(this);
    connect(slideshowbottombar, &SlideShowBottomBar::showPrevious, this, [ = ] {
        m_player->startPrevious();
    });
//    connect(slideshowbottombar, &SlideShowBottomBar::showPause, this, [=]{

//    });
    connect(slideshowbottombar, &SlideShowBottomBar::showNext, this, [ = ] {
        m_player->startNext();
    });
    connect(slideshowbottombar, &SlideShowBottomBar::showCancel, this, [ = ] {
        backToLastPanel();
    });
}

QString SlideShowPanel::moduleName()
{
    return "SlideshowPanel";
}

QWidget *SlideShowPanel::toolbarBottomContent()
{
    return NULL;
}

QWidget *SlideShowPanel::toolbarTopLeftContent()
{
    return NULL;
}

QWidget *SlideShowPanel::toolbarTopMiddleContent()
{
    return NULL;
}

QWidget *SlideShowPanel::extensionPanelContent()
{
    return NULL;
}

void SlideShowPanel::paintEvent(QPaintEvent *e)
{
    ModulePanel::paintEvent(e);

    if (m_img.isNull())
        return;

    QPainter p(this);
//    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(this->rect(), QBrush(QColor(m_bgColor)));
    p.drawPixmap(this->rect(), QPixmap::fromImage(m_img));
}

void SlideShowPanel::resizeEvent(QResizeEvent *e)
{
    m_player->setFrameSize(e->size().width() * devicePixelRatioF(),
                           e->size().height() * devicePixelRatioF());
    ModulePanel::resizeEvent(e);
}

void SlideShowPanel::backToLastPanel()
{
    m_player->stop();
    showNormal();

    if (FROM_MAINWINDOW_POPVIEW == m_vinfo.viewMainWindowID) {
        m_vinfo.path = m_player->currentImagePath();
        m_vinfo.fullScreen = false;
        m_vinfo.slideShow = false;
        emit dApp->signalM->viewImage(m_vinfo);
        emit dApp->signalM->hideSlidePanel();
    } else {
        emit dApp->signalM->hideSlidePanel();
    }

//    if (m_vinfo.lastPanel) {
//        ViewPanel *vp = dynamic_cast<ViewPanel *>(m_vinfo.lastPanel);
//        if (vp) {
//            m_vinfo.path = m_player->currentImagePath();
//            m_vinfo.lastPanel = vp->viewInfo().lastPanel;
//            emit dApp->signalM->viewImage(m_vinfo);

//            if (m_vinfo.fullScreen) {
//                emit dApp->signalM->hideTopToolbar(true);
//            }
//        }
//        else {
//            emit dApp->signalM->gotoPanel(m_vinfo.lastPanel);
//            emit dApp->signalM->showBottomToolbar();
//        }
//    }
//    else {
//        emit dApp->signalM->backToMainPanel();
//    }

    // Clear cache
    QImage ti(width(), height(), QImage::Format_ARGB32);
    ti.fill(0);
    setImage(ti);

    this->setCursor(Qt::ArrowCursor);
    killTimer(m_hideCursorTid);
    m_hideCursorTid = 0;
}

void SlideShowPanel::initeffectPlay()
{
    m_player = new SlideEffectPlayer(this);
    connect(m_player, &SlideEffectPlayer::frameReady,
            this, &SlideShowPanel::setImage);
}

void SlideShowPanel::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_menu);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    m_menu->addAction(ac);
}

void SlideShowPanel::initMenu()
{
//    this->setContextMenuPolicy(Qt::CustomContextMenu);

//    m_menu = new QMenu;
//    m_menu->setStyle(QStyleFactory::create("dlight"));

//    QString stopSc = dApp->setter->value(SHORTCUTVIEW_GROUP,
//                                         "Slide show").toString();
//    stopSc.replace(" ", "");
//    appendAction(IdStopslideshow, tr("End show"),
//                  stopSc);
//    appendAction(IdPlayOrPause, tr("Pause/Play"),
//                 QKeySequence(Qt::Key_Space).toString());
//    connect(m_menu, &QMenu::triggered, this, &SlideShowPanel::onMenuItemClicked);
//    connect(this, &SlideShowPanel::customContextMenuRequested, this, [=] {
//        m_menu->popup(QCursor::pos());
//    });
}

void SlideShowPanel::onMenuItemClicked(QAction *action)
{
    const int id = action->property("MenuID").toInt();
    switch (id) {
    case IdStopslideshow:
        backToLastPanel();
        break;
    case IdPlayOrPause:
        m_player->pause();
        break;
    default:
        break;
    }
}

void SlideShowPanel::initShortcut()
{
    // Esc
//    m_sEsc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
//    m_sEsc->setContext(Qt::WindowShortcut);
//    connect(m_sEsc, &QShortcut::activated, this, &SlideShowPanel::backToLastPanel);
}

void SlideShowPanel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::BackButton)
        m_sEsc->activated();
}

void SlideShowPanel::initFileSystemMonitor()
{
    m_fileSystemMonitor = new QFileSystemWatcher(this);

    connect(m_fileSystemMonitor, &QFileSystemWatcher::fileChanged, [ = ]
    (const QString & path) {
        if (!QFileInfo(path).exists()) {
            if (m_vinfo.paths.contains(path)) {
                m_vinfo.paths.removeOne(path);
                m_fileSystemMonitor->removePath(path);
            }
        }

        m_player->setImagePaths(m_vinfo.paths);
    });
}

void SlideShowPanel::timerEvent(QTimerEvent *event)
{
    // Delay to avoid fast switching causes the system to get stuck
    if (event->timerId() == m_startTid) {
        killTimer(m_startTid);
        m_startTid = 0;

        m_player->start();
        emit dApp->signalM->gotoPanel(this);

        showFullScreen();
    } else if (event->timerId() == m_hideCursorTid) {
        this->setCursor(Qt::BlankCursor);
    }

    ModulePanel::timerEvent(event);
}

void SlideShowPanel::mouseDoubleClickEvent(QMouseEvent *e)
{
    backToLastPanel();
}

void SlideShowPanel::contextMenuEvent(QContextMenuEvent *e)
{
    backToLastPanel();
}

void SlideShowPanel::mouseMoveEvent(QMouseEvent *e)
{
//    if (!(e->buttons() | Qt::NoButton)) {
    this->setCursor(Qt::ArrowCursor);
//        emit mouseHoverMoved();
//    } else {
//        this->setCursor(Qt::ClosedHandCursor);
//        emit transformChanged();
//    }

    if (window()->isFullScreen()) {
        QPoint pos = mapFromGlobal(QCursor::pos());
        if (height() - 20 < pos.y()
                && height() > pos.y()
                && height() == slideshowbottombar->y()) {
            QPropertyAnimation *animation = new QPropertyAnimation(slideshowbottombar, "pos");
            animation->setDuration(200);
            animation->setEasingCurve(QEasingCurve::NCurveTypes);
            animation->setStartValue(QPoint((width() - slideshowbottombar->width()) / 2, slideshowbottombar->y()));
            animation->setEndValue(QPoint((width() - slideshowbottombar->width()) / 2, height() - slideshowbottombar->height() - 10));
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        } else if (height() - slideshowbottombar->height() - 10 > pos.y()
                   && height() - slideshowbottombar->height() - 10 == slideshowbottombar->y()) {
            QPropertyAnimation *animation = new QPropertyAnimation(slideshowbottombar, "pos");
            animation->setDuration(200);
            animation->setEasingCurve(QEasingCurve::NCurveTypes);
            animation->setStartValue(QPoint((width() - slideshowbottombar->width()) / 2, slideshowbottombar->y()));
            animation->setEndValue(QPoint((width() - slideshowbottombar->width()) / 2, height()));
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }

}

void SlideShowPanel::setImage(const QImage &img)
{
    m_img = img;
    m_img.setDevicePixelRatio(devicePixelRatioF());

//    update();
    repaint();
}

void SlideShowPanel::startSlideShow(const SignalManager::ViewInfo &vinfo,
                                    bool inDB)
{
    if (vinfo.paths.isEmpty()) {
        qDebug() << "Start SlideShow failed! Paths is empty!";
        return;
    }

    m_vinfo = vinfo;
    m_player->setImagePaths(vinfo.paths);
    m_player->setCurrentImage(vinfo.path);

//    m_startTid = startTimer(DELAY_START_INTERVAL);

    this->setCursor(Qt::BlankCursor);
    m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);

    if (!inDB) {
        qDebug() << "startSlideShow fileMonitor";
        m_fileSystemMonitor->addPaths(m_vinfo.paths);
    }

//    if (m_cancelslideshow != nullptr) {
//        int nParentWidth = QApplication::desktop()->screenGeometry().width();

//        m_cancelslideshow->move(nParentWidth - 50, 0);

//        m_cancelslideshow->show();
//        m_cancelslideshow->raise();
//    }

//    emit dApp->signalM->initButton();

//    slideshowbottombar->playpauseButton(a);

    if (1 < vinfo.paths.length()) {
        slideshowbottombar->m_preButton->setEnabled(true);
        slideshowbottombar->m_nextButton->setEnabled(true);
        slideshowbottombar->m_playpauseButton->setEnabled(true);
        emit dApp->signalM->initButton();
    } else {
        slideshowbottombar->m_preButton->setEnabled(false);
        slideshowbottombar->m_nextButton->setEnabled(false);
        slideshowbottombar->m_playpauseButton->setEnabled(false);
        emit dApp->signalM->updatePauseButton();
    }

    int nParentWidth = QApplication::desktop()->screenGeometry().width();
    int nParentHeight = QApplication::desktop()->screenGeometry().height();
    slideshowbottombar->move((nParentWidth - slideshowbottombar->width()) / 2, nParentHeight);

    m_player->start();
//    emit dApp->signalM->gotoPanel(this);
    showFullScreen();
}

void SlideShowPanel::showNormal()
{
    if (m_isMaximized) {
        window()->showNormal();
        window()->showMaximized();
    } else {
        window()->showNormal();
    }
}

void SlideShowPanel::showFullScreen()
{
    m_isMaximized = window()->isMaximized();
    // Full screen then hide bars because hide animation depends on height()
    window()->showFullScreen();
//    showFullScreen();

    setImage(getFitImage(m_player->currentImagePath()));
    emit dApp->signalM->hideBottomToolbar(true);
    emit dApp->signalM->hideExtensionPanel(true);
    emit dApp->signalM->hideTopToolbar(true);
}

/*!
 * \brief SlideShowPanel::getFitImage
 * Compound pixmap to fill the blank after started.
 * \param path
 * \return
 */
QImage SlideShowPanel::getFitImage(const QString &path)
{
    QDesktopWidget *dw = dApp->desktop();
    const int dww = dw->screenGeometry(window()).width();
    const int dwh = dw->screenGeometry(window()).height();

    QImage ti(dww, dwh, QImage::Format_ARGB32);
    QImage image = utils::image::getRotatedImage(path);
    QRectF source(0.0, 0.0, image.width(), image.height());
    QRectF target;
    if (1.0 * dww / dwh > 1.0 * image.width() / image.height()) {
        const qreal w = 1.0 * image.width() * dwh / image.height();
        target = QRectF((dww - w) / 2, 0.0, w, dwh);
    } else {
        const qreal h = 1.0 * image.height() * dww / image.width();
        target = QRectF(0.0, (dwh - h) / 2, dww, h);
    }

    QPainter painter(&ti);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(0, 0, dww, dwh, m_bgColor);
    painter.drawImage(target, image, source);

    return ti;
}

void SlideShowPanel::onThemeChanged(ViewerThemeManager::AppTheme dark)
{
    if (dark == ViewerThemeManager::Dark) {
        m_bgColor = DARK_BG_COLOR;
    } else {
        m_bgColor = DARK_BG_COLOR;
    }
    update();
}

void SlideShowPanel::keyPressEvent(QKeyEvent *e)
{
//    if (Qt::Key_Space == e->key()) {
//        m_player->pause();
//    }
}

