#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aimassist.h"
#include "settings.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_isLaunchMode(true)
{
    m_ui = new Ui::MainWindow;
    m_ui->setupUi(this);

    m_dataHandler = new DataHandler(QDir::currentPath() + m_settings::dataFolder);
    m_dataHandler->loadData(&m_appPersistentData, &m_userPersistentData, &m_arrayPersistentData);

    m_statsScene = new QGraphicsScene();

    initialize();
    setStyleSheets();
}

MainWindow::~MainWindow()
{
    settings::isSaveData = m_ui->saveDataCheckBox->isChecked();
    settings::isViewJson = m_ui->viewJsonCheckBox->isChecked();

    m_appPersistentData.usersTableRowCount = m_ui->verticalSlider->value();
    m_appPersistentData.usersTableColumnCount = m_ui->horizontalSlider->value();
    m_appPersistentData.theme = m_ui->themes->currentText().toStdString();

    saveOsuRequestStats();

    m_dataHandler->saveData(m_appPersistentData, m_userPersistentData, m_arrayPersistentData);

    delete m_ui;
    delete m_dataHandler;
    delete m_statsScene;
}

void MainWindow::setStyleSheets()
{
    on_themes_currentIndexChanged(4);
    m_ui->jsonViewer->setStyleSheet("QTextBrowser { color: #ffffff; }");
}

void MainWindow::initialize()
{
    const int tableElementsCount = m_settings::tableSize * m_settings::tableSize;
    m_tableThreads = QList<QFuture<void>>(tableElementsCount);

    m_ui->saveDataCheckBox->setChecked(settings::isSaveData);

    m_ui->userTable->setColumnCount(m_settings::tableSize);
    m_ui->userTable->setRowCount(m_settings::tableSize);

    m_ui->verticalSlider->setValue(m_appPersistentData.usersTableRowCount);
    on_verticalSlider_valueChanged(m_appPersistentData.usersTableRowCount);

    m_ui->horizontalSlider->setValue(m_appPersistentData.usersTableColumnCount);
    on_horizontalSlider_valueChanged(m_appPersistentData.usersTableColumnCount);

    for (int i = 0; i < m_settings::themesSize; i++)
    {
        m_ui->themes->addItem(m_settings::themes[i]);
    }

    m_ui->themes->setCurrentText(QString::fromStdString(m_appPersistentData.theme));

    setTableMode();
    setTableUsers();
    setChooseUser();

    if (!settings::isViewJson)
    {
        m_ui->jsonViewer->hide();
    }
    else
    {
        m_ui->viewJsonCheckBox->setChecked(true);
    }

    m_ui->osuProcessNameLabel->setText(settings::osuClientName);
    m_ui->osuBotNameLabel->setText(settings::osuBotName);

    initOverviewImages();
    initOverviewGraphics();
    initOsuRequestStats();

    on_goChoosePage_pressed();
}

void MainWindow::on_goChoosePage_pressed()
{
    m_ui->contentViewer->setCurrentIndex(0);
    m_ui->goChoosePage->setStyleSheet(special::buttonActiveStyle);
    m_ui->goOverviewPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goAimAssistPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goSettingsPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goAboutAppPage->setStyleSheet(special::buttonInactiveStyle);
}

void MainWindow::on_goOverviewPage_pressed()
{
    if (!m_userPersistentData.isChooseUser)
    {
        QMessageBox::warning(nullptr, "Overview", "The player is not choosed");
        return;
    }

    const int playCount = m_osuParser.getPlayCount();
    if (playCount >= m_settings::playLimit)
    {
        initOverview();

        m_ui->contentViewer->setCurrentIndex(1);
        m_ui->goChoosePage->setStyleSheet(special::buttonInactiveStyle);
        m_ui->goOverviewPage->setStyleSheet(special::buttonActiveStyle);
        m_ui->goAimAssistPage->setStyleSheet(special::buttonInactiveStyle);
        m_ui->goSettingsPage->setStyleSheet(special::buttonInactiveStyle);
        m_ui->goAboutAppPage->setStyleSheet(special::buttonInactiveStyle);
    }
    else
    {
        QMessageBox::warning(nullptr, "Overview", "Insufficient number of maps played");
    }
}

void MainWindow::on_goAimAssistPage_pressed()
{
    m_ui->contentViewer->setCurrentIndex(2);
    m_ui->goChoosePage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goOverviewPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goAimAssistPage->setStyleSheet(special::buttonActiveStyle);
    m_ui->goSettingsPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goAboutAppPage->setStyleSheet(special::buttonInactiveStyle);
}

void MainWindow::on_goSettingsPage_pressed()
{
    m_ui->contentViewer->setCurrentIndex(3);
    m_ui->goChoosePage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goOverviewPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goAimAssistPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goSettingsPage->setStyleSheet(special::buttonActiveStyle);
    m_ui->goAboutAppPage->setStyleSheet(special::buttonInactiveStyle);
}

void MainWindow::on_goAboutAppPage_pressed()
{
    m_ui->contentViewer->setCurrentIndex(4);
    m_ui->goChoosePage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goOverviewPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goAimAssistPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goSettingsPage->setStyleSheet(special::buttonInactiveStyle);
    m_ui->goAboutAppPage->setStyleSheet(special::buttonActiveStyle);
}

void MainWindow::on_removeDataButton_pressed()
{
    m_dataHandler->deleteDataFile();
}

void MainWindow::loadUrlImage(QPixmap *pixmap, const QUrl &url)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(QNetworkRequest(url));

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if(reply->error() == QNetworkReply::NoError)
    {
        QByteArray imageData = reply->readAll();
        pixmap->loadFromData(imageData);
    }

    reply->deleteLater();
}

void MainWindow::setTableMode()
{
    if (m_appPersistentData.isTableModeAdd)
    {
        m_ui->tableModeButton->setText(special::tableModeAdd);
    }
    else
    {
        m_ui->tableModeButton->setText(special::tableModeRemove);
    }
}

void MainWindow::setTableUsers()
{
    if (!settings::isSaveData)
    {
        return;
    }

    for (int i = 0; i < m_settings::tableSize; i++)
    {
        for (int j = 0; j < m_settings::tableSize; j++)
        {
            const QString username = m_arrayPersistentData.users[i][j].username;
            if (!username.isEmpty())
            {
                const QPixmap avatar = m_arrayPersistentData.users[i][j].image;
                setUserTable(i, j, username, avatar);
            }
        }
    }
}

void MainWindow::setChooseUser()
{
    if (!m_userPersistentData.isChooseUser)
    {
        return;
    }

    const QString username = m_arrayPersistentData.chooseUser.username;
    const QPixmap image = m_arrayPersistentData.chooseUser.image;

    QLabel *uiUsername = m_ui->chooseUsername;
    QLabel *uiImage = m_ui->chooseImage;

    uiUsername->setText(username);
    uiUsername->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    uiImage->setPixmap(image.scaled(m_settings::pixmapSize, m_settings::pixmapSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    uiImage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

void MainWindow::setUserTable(int row, int column, const QString &username, const QPixmap &avatar)
{
    QWidget *player = new QWidget();
    QLabel *usernameLabel = new QLabel(username);

    QLabel *imageLabel = new QLabel();
    imageLabel->setPixmap(avatar.scaled(m_settings::pixmapSize, m_settings::pixmapSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    QVBoxLayout *layout = new QVBoxLayout(player);
    layout->addWidget(usernameLabel, 0, Qt::AlignHCenter | Qt::AlignTop);
    layout->addWidget(imageLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);

    QFont font = usernameLabel->font();
    font.setWeight(QFont::Bold);
    usernameLabel->setFont(font);

    m_ui->userTable->setCellWidget(row, column, player);
}

void MainWindow::waitCellOperation(int row, int column)
{
    const int index = row * m_settings::tableSize + column;
    m_tableThreads[index].waitForFinished();
}

void MainWindow::initOverviewImages()
{
    QPixmap pixmapsSS(":/Assets/ranking-XH.png");
    QPixmap scaledPixmapsSS = pixmapsSS.scaled(m_ui->SSHRankImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_ui->SSHRankImage->setAlignment(Qt::AlignCenter);
    m_ui->SSHRankImage->setPixmap(scaledPixmapsSS);

    QPixmap pixmapSS(":/Assets/ranking-X.png");
    QPixmap scaledPixmapSS = pixmapSS.scaled(m_ui->SSRankImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_ui->SSRankImage->setAlignment(Qt::AlignCenter);
    m_ui->SSRankImage->setPixmap(scaledPixmapSS);

    QPixmap pixmapsS(":/Assets/ranking-SH.png");
    QPixmap scaledPixmapsS = pixmapsS.scaled(m_ui->SHRankImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_ui->SHRankImage->setAlignment(Qt::AlignCenter);
    m_ui->SHRankImage->setPixmap(scaledPixmapsS);

    QPixmap pixmapS(":/Assets/ranking-S.png");
    QPixmap scaledPixmapS = pixmapS.scaled(m_ui->SRankImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_ui->SRankImage->setAlignment(Qt::AlignCenter);
    m_ui->SRankImage->setPixmap(scaledPixmapS);

    QPixmap pixmapA(":/Assets/ranking-A.png");
    QPixmap scaledPixmapA = pixmapA.scaled(m_ui->ARankImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_ui->ARankImage->setAlignment(Qt::AlignCenter);
    m_ui->ARankImage->setPixmap(scaledPixmapA);
}

void MainWindow::initOverview()
{
    initUserStats();
    drawStatsPolygon();

    const float userAccuracy = m_osuParser.getAccuracy();
    const float roundedAccuracy = int(userAccuracy * 100.0f) / 100.0f;

    m_ui->usernamelabel->setText(m_ui->chooseUsername->text());
    m_ui->picturelabel->setPixmap(m_ui->chooseImage->pixmap());
    m_ui->playcountlabel->setText(QString::number(m_osuParser.getPlayCount()));
    m_ui->globalranklabel->setText("#" + QString::number(m_osuParser.getGlobalRank()));
    m_ui->PPlabel->setText(QString::number(m_osuParser.getPpCount()) + "pp");
    m_ui->countrylabel->setText("#" + QString::number(m_osuParser.getCountryRank()));
    m_ui->accuracylabel->setText(QString::number(roundedAccuracy) + "%");

    const QString countryCode = m_osuParser.getCountryCode().toLower();
    const QUrl countryUrl("https://worldflags.net/assets/flaggor/flags/4x3/" + countryCode + ".svg");

    QPixmap countryPixmap;
    loadUrlImage(&countryPixmap, countryUrl);

    if (countryPixmap.isNull())
    {
        m_ui->usercountrylabel->setAlignment(Qt::AlignCenter);
        m_ui->usercountrylabel->setText(countryCode);
    }
    else
    {
        QPixmap scaledPixmapCountry = countryPixmap.scaled(m_ui->ARankImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        m_ui->usercountrylabel->setAlignment(Qt::AlignCenter);
        m_ui->usercountrylabel->setPixmap(scaledPixmapCountry);
    }
}

void MainWindow::initUserStats()
{
    const float cs = m_osuParser.getAvgCs();
    const float pp = m_osuParser.getAvgPP();
    const float ar = m_osuParser.getAvgAr();
    const float acc = m_osuParser.getAvgAcc();
    const float bpm = m_osuParser.getAvgBpm();
    const float length = qSqrt(m_osuParser.getAvgLength());

    m_aimValue = (ar * qPow(cs, 0.1) / qPow(6, 0.1) / qPow(ar + cs, 0.1) * pp / (ar + cs + pp) * 100) +
                (1.25 * qPow((cs + pp + ar + acc) / 4, 1.3));

    m_speedValue = (ar * qPow(ar, 0.1) / qPow(6, 0.1) / qPow(2 * ar, 0.1) * pp / (2 * ar + pp) * 100) +
                (1.25 * qPow((cs + pp + ar + acc) / 4, 1.3));

    m_accuracyValue = (ar * qPow(acc, 0.1) / qPow(6, 0.1) / qPow(ar + acc, 0.1) * pp / (ar + acc + pp) * 100) +
                (1.25 * qPow((cs + pp + ar + acc) / 4, 1.3));

    m_staminaValue = (ar * qPow(length, 0.1) / qPow(6, 0.1) / qPow(ar + bpm, 0.1) * pp / (ar + bpm + length + pp) * 100) +
                (1.25 * qPow((bpm + length + pp + ar + acc) / 5, 1.3));

    m_ui->aimValueLabel->setText(QString::number(m_aimValue));
    m_ui->speedValueLabel->setText(QString::number(m_speedValue));
    m_ui->accuracyValueLabel->setText(QString::number(m_accuracyValue));
    m_ui->staminaValueLabel->setText(QString::number(m_staminaValue));
}

void MainWindow::initOverviewGraphics()
{
    m_ui->graphicsView->setScene(m_statsScene);

    m_ui->graphicsView->setFrameStyle(QFrame::NoFrame);

    m_ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    m_ui->graphicsView->setRenderHint(QPainter::TextAntialiasing);

    m_ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QPolygon edging;
    edging << QPoint(0, m_graphics::polygonSize)
           << QPoint(m_graphics::polygonSize, 0)
           << QPoint(0, -m_graphics::polygonSize)
           << QPoint(-m_graphics::polygonSize, 0);

    m_statsScene->addPolygon(edging, QPen(m_graphics::penColor, m_graphics::polygonWidth), QBrush(m_graphics::polygonColor));

    m_statsScene->addLine(-m_graphics::polygonSize, 0, m_graphics::polygonSize, 0);
    m_statsScene->addLine(0, -m_graphics::polygonSize, 0, m_graphics::polygonSize);

    const int countStreaks = m_graphics::polygonSize / (m_graphics::streakOrientCount * 2) - 1;
    const int tab = m_graphics::polygonSize / m_graphics::streakOrientCount;

    for (int i = 0; i < countStreaks; i++)
    {
        m_statsScene->addLine(m_graphics::polygonSize - tab * (i + 1), -m_graphics::streakWidth,
                              m_graphics::polygonSize - tab * (i + 1), m_graphics::streakWidth);
        m_statsScene->addLine(-m_graphics::streakWidth, m_graphics::polygonSize - tab * (i + 1),
                              m_graphics::streakWidth, m_graphics::polygonSize - tab * (i + 1));
    }
}

void MainWindow::initOsuRequestStats()
{
    if (!m_userPersistentData.isChooseUser)
    {
        return;
    }

    m_osuParser.setUserId(m_userPersistentData.userId);
    m_osuParser.setPlayCount(m_userPersistentData.playCount);
    m_osuParser.setGlobalRank(m_userPersistentData.globalrank);
    m_osuParser.setCountryRank(m_userPersistentData.countryRank);
    m_osuParser.setPpCount(m_userPersistentData.pp);
    m_osuParser.setAccuracy(m_userPersistentData.accuracy);
    m_osuParser.setAvgCs(m_userPersistentData.avrCs);
    m_osuParser.setAvgPP(m_userPersistentData.avrPp);
    m_osuParser.setAvgAr(m_userPersistentData.avrAr);
    m_osuParser.setAvgAcc(m_userPersistentData.avrAccuracy);
    m_osuParser.setAvgBpm(m_userPersistentData.avrBpm);
    m_osuParser.setAvgLength(m_userPersistentData.avrLength);
    m_osuParser.setCountryCode(m_userPersistentData.countryCode);
}

void MainWindow::saveOsuRequestStats()
{
    if (!settings::isSaveData)
    {
        return;
    }

    m_userPersistentData.userId = m_osuParser.getUserId();
    m_userPersistentData.playCount = m_osuParser.getPlayCount();
    m_userPersistentData.globalrank = m_osuParser.getGlobalRank();
    m_userPersistentData.countryRank = m_osuParser.getCountryRank();
    m_userPersistentData.pp = m_osuParser.getPpCount();
    m_userPersistentData.accuracy = m_osuParser.getAccuracy();
    m_userPersistentData.avrCs = m_osuParser.getAvgCs();
    m_userPersistentData.avrPp = m_osuParser.getAvgPP();
    m_userPersistentData.avrAr = m_osuParser.getAvgAr();
    m_userPersistentData.avrAccuracy = m_osuParser.getAvgAcc();
    m_userPersistentData.avrBpm = m_osuParser.getAvgBpm();
    m_userPersistentData.avrLength = m_osuParser.getAvgLength();
    m_userPersistentData.countryCode = m_osuParser.getCountryCode();
}

void MainWindow::drawStatsPolygon()
{
    const int aimValue = m_aimValue * m_graphics::statsFactor;
    const int staminaValue = m_staminaValue * m_graphics::statsFactor;
    const int speedValue = m_speedValue * m_graphics::statsFactor;
    const int accuracyValue = m_accuracyValue * m_graphics::statsFactor;

    const QPoint aimPoint(0, -qMin(m_graphics::polygonSize, aimValue));
    const QPoint staminaPoint(0, qMin(m_graphics::polygonSize, staminaValue));
    const QPoint speedPoint(qMin(m_graphics::polygonSize, speedValue), 0);
    const QPoint accuracyPoint(-qMin(m_graphics::polygonSize, accuracyValue), 0);

    QPolygon statPolygon;
    statPolygon << aimPoint << speedPoint << staminaPoint << accuracyPoint;
    m_statsScene->addPolygon(statPolygon, QPen(m_graphics::pointColor, m_graphics::polygonWidth), QBrush(m_graphics::pointColor));
}

QString MainWindow::getUserJsonFromUsername(const QString &username)
{
    for (int i = 0; i < m_settings::tableSize; i++)
    {
        for (int j = 0; j < m_settings::tableSize; j++)
        {
            const QString cellUsername = m_arrayPersistentData.users[i][j].username;
            if (cellUsername == username)
            {
                return m_arrayPersistentData.users[i][j].userInfo;
            }
        }
    }

    return QString();
}

void MainWindow::addCell(int row, int column)
{    
    bool isOk;
    const QString input = QInputDialog::getText(nullptr, "Add a new player...", "Enter a nickname:", QLineEdit::Normal, "username", &isOk);
    if (!isOk)
    {
        return;
    }

    if (input.isEmpty() || !m_osuParser.setPlayer(input))
    {
        QMessageBox::warning(nullptr, "Add a new player...", "Couldn't find a player");
        return;
    }

    QFuture<void> future = QtConcurrent::run([=]()
    {
        m_arrayPersistentData.users[row][column].userScoresInfo =
                QString::fromUtf8(m_osuParser.getUserScores(m_osuParser.getUserId()).toJson());
        m_osuParser.calculateAvr();
    });

    const int index = row * m_settings::tableSize + column;
    m_tableThreads[index].cancel();
    m_tableThreads[index] = future;

    const QString username = m_osuParser.getUsername();
    const QString userInfo = m_osuParser.getUserInfo();
    const QString avatarUrl = m_osuParser.getAvatarUrl();

    m_ui->jsonViewer->setText(userInfo);

    QPixmap avatar;
    loadUrlImage(&avatar, avatarUrl);

    m_arrayPersistentData.users[row][column].username = username;
    m_arrayPersistentData.users[row][column].userInfo = userInfo;
    m_arrayPersistentData.users[row][column].image = avatar;

    setUserTable(row, column, username, avatar);
}

void MainWindow::clearCell(int row, int column)
{
    m_arrayPersistentData.users[row][column].username = QString();
    m_ui->userTable->setCellWidget(row, column, nullptr);
    m_ui->jsonViewer->clear();
}

void MainWindow::chooseCell(int row, int column)
{
    const QString username = m_arrayPersistentData.users[row][column].username;
    QLabel *uiUsername = m_ui->chooseUsername;

    if (username == uiUsername->text())
    {
        return;
    }

    const QPixmap image = m_arrayPersistentData.users[row][column].image;
    QLabel *uiImage = m_ui->chooseImage;

    uiUsername->setText(username);
    uiUsername->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    uiImage->setPixmap(image.scaled(m_settings::pixmapSize, m_settings::pixmapSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    uiImage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    waitCellOperation(row, column);

    m_arrayPersistentData.chooseUser.username = username;
    m_arrayPersistentData.chooseUser.image = image;
    m_arrayPersistentData.chooseUser.userInfo = m_arrayPersistentData.users[row][column].userInfo;
    m_arrayPersistentData.chooseUser.userScoresInfo = m_arrayPersistentData.users[row][column].userScoresInfo;

    m_osuParser.setUserJson(m_arrayPersistentData.chooseUser.userInfo);
    m_osuParser.setUserScoresJson(m_arrayPersistentData.chooseUser.userScoresInfo);

    m_userPersistentData.isChooseUser = true;
}

void MainWindow::on_viewJsonCheckBox_stateChanged(int state)
{
    if (state == Qt::Unchecked)
    {
        m_ui->jsonViewer->hide();
    }
    else
    {
        m_ui->jsonViewer->show();
    }
}

void MainWindow::on_userTable_cellClicked(int row, int column)
{
    const QTableWidget *table = m_ui->userTable;

    const QWidget *widget = table->cellWidget(row, column);
    if (widget)
    {
        QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(widget->layout());
        if (layout)
        {
            const QLabel *usernameLabel = qobject_cast<QLabel*>(layout->itemAt(0)->widget());
            const QString username = usernameLabel->text();

            m_ui->jsonViewer->setText(getUserJsonFromUsername(username));

            return;
        }
    }

    m_ui->jsonViewer->clear();
}

void MainWindow::on_userTable_cellDoubleClicked(int row, int column)
{
    const QWidget *widget = m_ui->userTable->cellWidget(row, column);
    if (m_appPersistentData.isTableModeAdd)
    {
        if (widget == nullptr)
        {
            addCell(row, column);
        }
        else
        {
            chooseCell(row,column);
        }
    }
    else
    {
        clearCell(row, column);
    }
}

void MainWindow::on_verticalSlider_valueChanged(int value)
{
    for (int i = 0; i < value; i++)
    {
        m_ui->userTable->showColumn(i);
    }

    for (int i = value; i < m_settings::tableSize; i++)
    {
        m_ui->userTable->hideColumn(i);
    }
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    for (int i = 0; i < value; i++)
    {
        m_ui->userTable->showRow(i);
    }

    for (int i = value; i < m_settings::tableSize; i++)
    {
        m_ui->userTable->hideRow(i);
    }
}

void MainWindow::on_tableModeButton_pressed()
{
    m_appPersistentData.isTableModeAdd = !m_appPersistentData.isTableModeAdd;
    setTableMode();
}

void MainWindow::on_LaunchAssist_pressed()
{
    settings::osuClientName = m_ui->osuProcessNameLabel->text();
    settings::osuBotName = m_ui->osuBotNameLabel->text();

    if (settings::osuClientName.isEmpty() || settings::osuBotName.isEmpty())
    {
        QMessageBox::warning(this, "Aim Assist", "Fields is empty");
        return;
    }

    AppHandler m_osuApp;
    if (m_osuApp.getStatus() != AppHandler::ResultCode::success)
    {
        QMessageBox::warning(this, "Aim Assist", "osu! not running");
        return;
    }

    m_isLaunchMode = !m_isLaunchMode;
    if (m_isLaunchMode)
    {
        m_assistThread.cancel();

        m_ui->LaunchAssist->setText("Launch");
        m_ui->LaunchAssist->setStyleSheet(special::buttonInactiveStyle);
    }
    else
    {
        m_assistThread = QtConcurrent::run([=]()
        {
            assist::run();
        });

        m_ui->LaunchAssist->setText("Stop");
        m_ui->LaunchAssist->setStyleSheet(special::buttonActiveStyle);
    }
}


void MainWindow::on_themes_currentIndexChanged(int index)
{
    m_appPersistentData.theme = m_settings::themes[index].toStdString();

    QFile file(":/Style/" + QString::fromStdString(m_appPersistentData.theme) + ".qss");
    file.open(QFile::ReadOnly);

    const QString styleSheet { QLatin1String(file.readAll()) };
    this->setStyleSheet(styleSheet);

    file.close();
}

