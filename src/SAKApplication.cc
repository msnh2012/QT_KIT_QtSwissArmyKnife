﻿/******************************************************************************
 * Copyright 2018-2022 Qter(qsaker@qq.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part
 * of QtSwissArmyKnife project.
 *
 * QtSwissArmyKnife is licensed according to the terms in
 * the file LICENCE in the root of the source code directory.
 *****************************************************************************/
#include <QRect>
#include <QFile>
#include <QTimer>
#include <QDebug>
#include <QLabel>
#include <QAction>
#include <QDialog>
#include <QDateTime>
#include <QSettings>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif
#include <QSizePolicy>
#include <QGridLayout>
#include <QTextCursor>
#include <QTranslator>
#include <QPushButton>
#include <QStyleFactory>
#include <QStandardPaths>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#endif

#include "SAKApplication.hh"

QDate buildDate = QLocale(QLocale::English).toDate(
            QString(__DATE__).replace("  ", " 0"),
            "MMM dd yyyy");
QTime buildTime = QTime::fromString(__TIME__, "hh:mm:ss");
SAKApplication::SAKApplication(int argc, char **argv)
    :QApplication (argc, argv)
{
    // Initialize some information about application.
    setOrganizationName(QString("Qter"));
    setOrganizationDomain(QString("IT"));
    setApplicationName(QString("QtSwissArmyKnife"));
#ifdef SAK_VERSION
    setApplicationVersion(SAK_VERSION);
#else
    setApplicationVersion("0.0.0");
#endif


    // It can avoid app crash in this way to show a splashScreen.
    // If you new a QSplashScreen and show it in the main function,
    // app will crash(test on Ubuntu 16.04).
    // Of course, it is because that I use a wrong way,
    // also, it could be a bug of Qt.
    QPixmap pixmap(":/resources/images/StartUi.jpg");
    mSplashScreen = new QSplashScreen(pixmap);
    showSplashScreenMessage(tr("Initializing..."));
    mSplashScreen->show();
    processEvents();


    // Initialize the setting key
    mSettingsKeyContext.lastDateTime = QString("lastDateTime");
    mSettingsKeyContext.removeSettingsFile = QString("removeSettingsFile");
    mSettingsKeyContext.removeDatabase = QString("removeDatabase");
    mSettingsKeyContext.language = QString("language");
    mSettingsKeyContext.appStyle = QString("appStyle");


    // Initialize the settings file
    QStandardPaths::StandardLocation type = QStandardPaths::AppConfigLocation;
    QString path = QStandardPaths::writableLocation(type);
    mSettingsFileName = QString("%1/%2.ini").arg(path,
                                                 qApp->applicationName());
    mSettings = new QSettings(mSettingsFileName, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mSettings->setIniCodec(QTextCodec::codecForName("UTF-8"));
#endif
    QString key = mSettingsKeyContext.lastDateTime;
    QDateTime dt = QDateTime::currentDateTime();
    mLastDataTime = mSettings->value(key).toString();
    mSettings->setValue(mSettingsKeyContext.lastDateTime,
                        dt.toString(QLocale::system().dateFormat()));

    // Initialize the data base
    mDatabaseName = QString("%1/%2.sqlite3")
            .arg(path, qApp->applicationName());

    // Remove settings file and database
    if (mSettings->value(mSettingsKeyContext.removeSettingsFile).toBool()){
        mSettings->setValue(mSettingsKeyContext.removeSettingsFile,
                            QVariant::fromValue(false));
        if (QFile::remove(mSettingsFileName)){
            qInfo() << "Remove settings file successfully!";
        }else{
            qWarning() << "Remove settings file failed!";
        }

        QFile databaseFile(mDatabaseName);
        if (databaseFile.remove()){
            qInfo() << "Remove database successfully!";
        }else{
            qWarning() << "Remove database failed: "
                       << databaseFile.errorString();
        }
    }

    // Readin setting info, set the most beautiful style for the paltform.
    if (mSettings->value(mSettingsKeyContext.appStyle).toString().isEmpty()){
        const QString defauleStyle = QString(SAK_STYLE_DEFAULT);
        auto styleKeys = QStyleFactory::keys();
        if (styleKeys.contains(defauleStyle)){
            setStyle(defauleStyle);
            mSettings->setValue(mSettingsKeyContext.appStyle, defauleStyle);
        }
    }


    // Initialize database.
    mSqlDatabase = QSqlDatabase::addDatabase("QSQLITE");
    mSqlDatabase.setDatabaseName(mDatabaseName);
    // Do something useless.
    mSqlDatabase.setHostName("localhost");
    mSqlDatabase.setUserName("Qter");
    mSqlDatabase.setPassword("QterPassword");
    if (!mSqlDatabase.open()){
        qWarning() << "QSAKDatabase.sqlite3 open failed:"
                   << mSqlDatabase.lastError().text();
        Q_ASSERT_X(false, __FUNCTION__, "Open database failed! Using release edition please!");
    }


    // Setup ui language.
    installLanguage();
}

SAKApplication::~SAKApplication()
{
    if (mSqlDatabase.isOpen()){
        mSqlDatabase.close();
    }
}

void SAKApplication::installLanguage()
{
    QString key = mSettingsKeyContext.language;
    QString language = mSettings->value(key).toString();
    QString qmName;
    if (language.isEmpty()) {
        if (QLocale().country() == QLocale::China){
            qmName = QString("zh_CN");
            mSettings->setValue(mSettingsKeyContext.language,
                                QString("zh_CN-简体中文"));
        } else {
            qmName = QString("en");
            mSettings->setValue(mSettingsKeyContext.language,
                                QString("en-English"));
        }
    } else {
        qmName = language.split('-').first();
    }

    QString qm = QString(":/translations/qt/qtbase_%1.qm").arg(qmName);
    auto ret = mQtBaseTranslator.load(qm);
    Q_UNUSED(ret);
    qApp->installTranslator(&mQtBaseTranslator);

    qm = QString(":/translations/qt/qt_%1.qm").arg(qmName);
    ret = mQtTranslator.load(qm);
    Q_UNUSED(ret);
    qApp->installTranslator(&mQtTranslator);

    qm = QString(":/translations/sak/SAK_%1.qm").arg(qmName);
    ret = mSakTranslator.load(qm);
    Q_UNUSED(ret);
    qApp->installTranslator(&mSakTranslator);
}

const QDate *SAKApplication::buildDate()
{
    return &::buildDate;
}

const QTime* SAKApplication::buildTime()
{
    return &::buildTime;
}

QString SAKApplication::dataPath()
{
    QString path = mSettings->fileName();
    QStringList temp = path.split('/');
    path = path.remove(temp.last());

    return path;
}

QSettings *SAKApplication::settings()
{
    return mSettings;
}

SAKApplication::SettingsKeyContext *SAKApplication::settingsKeyContext()
{
    return &mSettingsKeyContext;
}

QSplashScreen *SAKApplication::splashScreen()
{
    return mSplashScreen;
}

void SAKApplication::showSplashScreenMessage(QString msg)
{
    mSplashScreen->showMessage(msg, Qt::AlignBottom, QColor(255, 255, 255));
}

QSqlDatabase *SAKApplication::sqlDatabase()
{
    return &mSqlDatabase;
}
