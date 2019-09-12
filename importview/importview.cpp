#include "importview.h"

namespace {
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
}  //namespace

ImportView::ImportView()
{
    setAcceptDrops(true);

    initUI();
    initConnections();
}

void ImportView::initConnections()
{

}

void ImportView::initUI()
{
    QVBoxLayout* pImportFrameLayout = new QVBoxLayout();

    DLabel* pLabel = new DLabel();
    pLabel->setFixedSize(128, 128);

    QPixmap pixmap;
    pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));

    pLabel->setPixmap(pixmap);

    m_pImportBtn = new DPushButton();
    m_pImportBtn->setText("导入图片");
    m_pImportBtn->setFixedSize(142, 42);

    DLabel* pLabel2 = new DLabel();
    pLabel2->setFixedHeight(24);
    pLabel2->setText("您也可以拖拽或导入图片到时间线");

    pImportFrameLayout->addStretch();
    pImportFrameLayout->addWidget(pLabel, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(20);
    pImportFrameLayout->addWidget(m_pImportBtn, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(10);
    pImportFrameLayout->addWidget(pLabel2, 0, Qt::AlignCenter);
    pImportFrameLayout->addStretch();

    setLayout(pImportFrameLayout);
}

void ImportView::dragEnterEvent(QDragEnterEvent *e)
{
    e->setDropAction(Qt::CopyAction);
    e->accept();
//    ImportView::dragEnterEvent(e);
}

void ImportView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }

    using namespace utils::image;
    QStringList paths;
    for (QUrl url : urls) {
        const QString path = url.toLocalFile();
        if (QFileInfo(path).isDir()) {
            auto finfos =  getImagesInfo(path, false);
            for (auto finfo : finfos) {
                if (imageSupportRead(finfo.absoluteFilePath())) {
                    paths << finfo.absoluteFilePath();
                }
            }
        } else if (imageSupportRead(path)) {
            paths << path;
        }
    }

    if (paths.isEmpty())
    {
        return;
    }

    DBImgInfoList dbInfos;

    using namespace utils::image;

    for (auto path : paths)
    {
//        if (! imageSupportRead(imagePath)) {
//            continue;
//        }

        // Generate thumbnail and storage into cache dir
        if (! utils::image::thumbnailExist(path)) {
            // Generate thumbnail failed, do not insert into DB
            if (! utils::image::generateThumbnail(path)) {
                continue;
            }
        }

        QFileInfo fi(path);
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = path;
        dbi.dirHash = utils::base::hash(QString());
        dbi.time = fi.birthTime();

        dbInfos << dbi;
    }

    if (! dbInfos.isEmpty())
    {
        emit dApp->signalM->sigImprotPicsIntoDB(dbInfos);
    }

    event->accept();
}

void ImportView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void ImportView::dragLeaveEvent(QDragLeaveEvent *e)
{

}
