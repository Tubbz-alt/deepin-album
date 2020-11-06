#include "application.h"
#include "mainwindow.h"
#include "dtktest.h"
#include "imageengine/imageengineapi.h"
#include "accessibledefine.h"
#include "accessible.h"
#include <DMainWindow>
#include <DWidgetUtil>
#include <DApplicationSettings>
#include <DLog>
#include <QMessageBox>


DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE


QUrl UrlInfo(QString path)
{
    QUrl url;
    // Just check if the path is an existing file.
    if (QFile::exists(path)) {
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
        return url;
    }

    const auto match = QRegularExpression(QStringLiteral(":(\\d+)(?::(\\d+))?:?$")).match(path);

    if (match.isValid()) {
        // cut away line/column specification from the path.
        path.chop(match.capturedLength());
    }

    // make relative paths absolute using the current working directory
    // prefer local file, if in doubt!
    url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);

    // in some cases, this will fail, e.g.
    // assume a local file and just convert it to an url.
    if (!url.isValid()) {
        // create absolute file path, we will e.g. pass this over dbus to other processes
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
    }
    return url;
}

int main(int argc, char *argv[])
{
    Application a(argc, argv);

    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    QAccessible::installFactory(accessibleFactory);
    a.setOrganizationName("deepin");
    a.setApplicationName("deepin-album");

    qputenv("DTK_USE_SEMAPHORE_SINGLEINSTANCE", "1");

    QCommandLineParser parser;
    parser.process(a);

    QStringList urls;
    QStringList arguments = parser.positionalArguments();

    QString filepath = "";
    bool bneedexit = true;
    for (const QString &path : arguments) {
        filepath = UrlInfo(path).toLocalFile();


        QFileInfo info(filepath);
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
        QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
        qDebug() << info.filePath() << "&&&&&&&&&&&&&&" << "mt" << mt.name() << "mt1" << mt1.name();

        QString str = info.suffix().toLower();
        if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")
                || mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                bneedexit = false;
                break;
            } else if (str.isEmpty()) {
                bneedexit = false;
                break;
            }
        }
    }
    if ("" != filepath && bneedexit) {
        exit(0);
    }

    if (!bneedexit) {
        if (bfirstopen) {
            bfirstandviewimage = true;
        }
    }

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();
    if (!DGuiApplicationHelper::instance()->setSingleInstance(a.applicationName(), DGuiApplicationHelper::UserScope)) {
        exit(0);
    }

    // LMH0420判断是否相同进程启动
    if (a.isRunning()) {
        return 0;
    }
    DBManager::instance();
    ImageEngineApi::instance(&a);
    ImageEngineApi::instance()->load80Thumbnails();
    MainWindow w;

    w.show();
    Dtk::Widget::moveToCenter(&w);

    //外部点击图片启动
    if ("" != filepath) {
        emit dApp->signalM->showImageView(0);
    }

//    qDebug() << "相册启动总耗时：" << t.elapsed();

    if (bneedexit)
        bfirstopen = false;

    return a.exec();
}
