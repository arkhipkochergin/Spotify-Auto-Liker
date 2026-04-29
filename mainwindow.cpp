#include "mainwindow.h"
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QMessageBox>
#include <QJsonArray>
#include <QTimer>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSettings>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);

    clientId = settings.value("Spotify/client_id", "").toString();
    clientSecret = settings.value("Spotify/client_secret", "").toString();

    if (clientId.isEmpty() || clientSecret.isEmpty()) {
        settings.setValue("Spotify/client_id", "insert_your_id_here");
        settings.setValue("Spotify/client_secret", "insert_your_secret_here");
        settings.sync();

        QMessageBox::information(this, "Config",
                                 "Файл config.ini создан рядом с .exe. Впишите туда ваши данные Spotify.");
    }

    setupUI();

    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(baseDir);
    dir.mkpath(".");
    actionsDir = dir.filePath("actions");
    QDir().mkpath(actionsDir);

    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
}

void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);

    btnConnect = new QPushButton("Connect to Spotify", this);
    btnConnect->setMinimumHeight(50);
    btnConnect->setStyleSheet("background-color: #1DB954; color: white; font-weight: bold; border-radius: 10px;");
    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    mainLayout->addWidget(btnConnect);
    mainLayout->addSpacing(20);

    QLabel *modeLabel = new QLabel("Select input method:", this);
    mainLayout->addWidget(modeLabel);

    radioFile = new QRadioButton("Export from .txt", this);
    radioText = new QRadioButton("Export from list", this);
    mainLayout->addWidget(radioFile);
    mainLayout->addWidget(radioText);

    btnRecentActions = new QPushButton("Recent actions", this);
    btnRecentActions->setMinimumHeight(28);
    btnRecentActions->setStyleSheet(
        "background-color: #ffffff; border: 1px solid #aaaaaa; color: #333333; font-weight: bold;"
        );
    connect(btnRecentActions, &QPushButton::clicked, this, &MainWindow::onRecentActionsClicked);
    mainLayout->addWidget(btnRecentActions);

    connect(radioFile, &QRadioButton::toggled, this, &MainWindow::onModeChanged);
    connect(radioText, &QRadioButton::toggled, this, &MainWindow::onModeChanged);

    mainLayout->addSpacing(10);

    btnRemoveFile = new QPushButton("Remove file", this);
    btnRemoveFile->setStyleSheet("color: #eb4034; border: 1px solid #eb4034; font-size: 11px; padding: 2px;");
    btnRemoveFile->setFixedWidth(100);
    btnRemoveFile->setVisible(false);

    connect(btnRemoveFile, &QPushButton::clicked, this, &MainWindow::onRemoveFileClicked);

    btnChooseFile = new QPushButton("Choose .txt file", this);
    lblFileName = new QLabel("No file selected", this);
    btnChooseFile->setVisible(false);
    lblFileName->setVisible(false);

    mainLayout->addWidget(btnRemoveFile);

    connect(btnChooseFile, &QPushButton::clicked, this, &MainWindow::onChooseFileClicked);

    mainLayout->addWidget(btnChooseFile);
    mainLayout->addWidget(lblFileName);

    txtEditor = new QTextEdit(this);
    txtEditor->setPlaceholderText("Paste your list here: \nArtist - Song\nArtist 2 - Song 2");
    txtEditor->setVisible(false);
    mainLayout->addWidget(txtEditor);

    mainLayout->addStretch();
    setCentralWidget(centralWidget);
    setWindowTitle("Spotify Liker");
    resize(400, 500);

    btnStartExport = new QPushButton("Start Export", this);
    btnStartExport->setMinimumHeight(40);
    btnStartExport->setStyleSheet("background-color: #ffffff; border: 1px solid #1DB954; color: #1DB954; font-weight: bold;");
    connect(btnStartExport, &QPushButton::clicked, this, &MainWindow::onStartExportClicked);
    mainLayout->addWidget(btnStartExport);

    //Delete all liked tracks
    btnDeleteAllTracks = new QPushButton("Delete all tracks", this);
    btnDeleteAllTracks->setMinimumHeight(45);
    btnDeleteAllTracks->setStyleSheet(
        "background-color: #eb4034; color: white; font-weight: bold; font-size: 14px; border-radius: 22px;"
        );
    connect(btnDeleteAllTracks, &QPushButton::clicked, this, &MainWindow::onDeleteAllTracksClicked);
    mainLayout->addWidget(btnDeleteAllTracks);

    progressBar = new QProgressBar(this);
    progressBar->setMinimumHeight(25);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 1px solid #aaaaaa;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   color: black;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #1DB954;"
        "   border-radius: 4px;"
        "}"
        );
    progressBar->setVisible(false);
    mainLayout->addWidget(progressBar);
    btnLikeTracks = new QPushButton("Like my tracks", this);
    btnLikeTracks->setMinimumHeight(50);
    btnLikeTracks->setStyleSheet("background-color: #1DB954; color: white; font-weight: bold; font-size: 16px; border-radius: 25px;");
    connect(btnLikeTracks, &QPushButton::clicked, this, &MainWindow::onLikeTracksClicked);
    mainLayout->addWidget(btnLikeTracks);

    networkManager = new QNetworkAccessManager(this);
}

void MainWindow::onConnectClicked() {
    if (!server->isListening()) {
        server->listen(QHostAddress::LocalHost, 8888);
    }
    QUrl authUrl("https://accounts.spotify.com/authorize");
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("redirect_uri", redirectUri);

    query.addQueryItem("scope", "user-library-read user-library-modify");

    query.addQueryItem("show_dialog", "true");
    authUrl.setQuery(query);
    QDesktopServices::openUrl(authUrl);
}

void MainWindow::onNewConnection() {
    QTcpSocket *socket = server->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, [this, socket]() {
        QByteArray data = socket->readAll();
        QString request(data);

        if (request.contains("GET /callback?code=")) {
            int start = request.indexOf("code=") + 5;
            int end = request.indexOf(" ", start);
            QString authCode = request.mid(start, end - start);

            socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
                          "<h1>Success!</h1><p>You can close this tab and return to the app.</p>");
            socket->disconnectFromHost();
            server->close();

            qDebug() << "Authorization Code received:" << authCode;
            exchangeCodeForToken(authCode);
            btnConnect->setText("Connected!");
            btnConnect->setEnabled(false);
        }
    });
}

void MainWindow::exchangeCodeForToken(QString code) {
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QUrl url("https://accounts.spotify.com/api/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("grant_type", "authorization_code");
    params.addQueryItem("code", code);
    params.addQueryItem("redirect_uri", redirectUri);
    params.addQueryItem("client_id", clientId);
    params.addQueryItem("client_secret", clientSecret);

    connect(manager, &QNetworkAccessManager::finished, [this, manager](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument json = QJsonDocument::fromJson(response);
            accessToken = json.object().value("access_token").toString();

            qDebug() << "Success! Access Token received.";
            btnConnect->setText("Authorized!");
        } else {
            qDebug() << "API Error:" << reply->errorString();
            qDebug() << "Response body:" << reply->readAll();
            btnConnect->setText("Auth Failed");
        }
        reply->deleteLater();
        manager->deleteLater();
    });

    manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
}

void MainWindow::onModeChanged() {
    bool isFile = radioFile->isChecked();
    btnChooseFile->setVisible(isFile);
    lblFileName->setVisible(isFile);

    btnRemoveFile->setVisible(isFile && !fullFilePath.isEmpty());

    txtEditor->setVisible(!isFile && radioText->isChecked());

    if (radioFile->isChecked()) {
        btnChooseFile->setVisible(true);
        lblFileName->setVisible(true);
        txtEditor->setVisible(false);
    } else if (radioText->isChecked()) {
        btnChooseFile->setVisible(false);
        lblFileName->setVisible(false);
        txtEditor->setVisible(true);
    }
}

void MainWindow::onChooseFileClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select File", "", "Text Files (*.txt)");

    if (!fileName.isEmpty()) {
        fullFilePath = fileName;
        lblFileName->setText("Selected: " + fileName.section('/', -1));
        btnRemoveFile->setVisible(true);
    }
}

void MainWindow::parseLine(QString line) {
    line = line.trimmed();
    if (line.isEmpty()) return;

    if (line.contains(" - ")) {
        QStringList parts = line.split(" - ");
        if (parts.size() >= 2) {
            Track track;
            track.artist = parts[0].trimmed().remove('\"');
            track.title = parts[1].trimmed().remove('\"');

            if (!track.artist.isEmpty() && !track.title.isEmpty()) {
                trackList.append(track);
            }
        }
    }
}

void MainWindow::onStartExportClicked() {
    trackList.clear();

    if (radioFile->isChecked()) {
        if (fullFilePath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please select a .txt file first!");
            return;
        }

        QFile file(fullFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setEncoding(QStringConverter::Utf8);

            while (!in.atEnd()) {
                parseLine(in.readLine());
            }
            file.close();
        } else {
            QMessageBox::critical(this, "Error", "Could not open file: " + file.errorString());
            return;
        }
    }
    else if (radioText->isChecked()) {
        QStringList lines = txtEditor->toPlainText().split('\n');
        for (const QString &line : lines) {
            parseLine(line);
        }
    }

    if (trackList.isEmpty()) {
        QMessageBox::information(this, "Result", "No valid tracks found. Check the format: Artist - Song");
    } else {
        QMessageBox::information(this, "Result",
                                 QString("Successfully parsed %1 tracks!").arg(trackList.size()));
    }
}

void MainWindow::onRemoveFileClicked() {
    fullFilePath.clear();
    lblFileName->setText("No file selected");
    btnRemoveFile->setVisible(false);
    qDebug() << "File selection cleared.";
}

QString MainWindow::makeLikeLine(const Track &t) const {
    return QString("\"%1\" - \"%2\"").arg(t.artist, t.title);
}

QString MainWindow::makeFailedLikeLine(const Track &t) const {
    return makeLikeLine(t) + " - Failed";
}

QString MainWindow::markFailedLine(const QString &okLine) const {
    QString s = okLine.trimmed();
    if (s.endsWith(" - Failed")) return s;
    return s + " - Failed";
}

void MainWindow::onLikeTracksClicked() {
    likedTrackLines.clear();
    likedSuccessCount = 0;
    likedFailCount = 0;

    pendingTrackIds.clear();
    pendingTrackLines.clear();
    if (accessToken.isEmpty()) {
        QMessageBox::warning(this, "Auth Error", "Please connect to Spotify first!");
        return;
    }
    if (trackList.isEmpty()) {
        QMessageBox::warning(this, "Data Error", "Track list is empty. Parse some tracks first!");
        return;
    }

    if (deleteInProgress) {
        QMessageBox::warning(this, "Busy", "Delete operation is in progress. Please wait until it finishes.");
        return;
    }

    // Reset per-run statistics
    likedTrackLines.clear();
    likedSuccessCount = 0;
    likedFailCount = 0;

    currentTrackIndex = 0;
    btnLikeTracks->setEnabled(false);
    btnLikeTracks->setText("Processing...");

    btnDeleteAllTracks->setEnabled(false);
    btnStartExport->setEnabled(false);

    progressBar->setRange(0, trackList.size());
    progressBar->setValue(0);
    progressBar->setVisible(true);

    processNextTrack();
}

void MainWindow::processNextTrack() {
    if (currentTrackIndex >= trackList.size()) {
        if (!pendingTrackIds.isEmpty()) {
            flushPendingLikes();
            return;
        }
        QString filePath;
        const QString baseName = makeActionFileBaseName("Liked");
        const bool ok = saveActionFile(baseName, likedTrackLines, &filePath);

        btnLikeTracks->setEnabled(true);
        btnLikeTracks->setText("Like my tracks");
        btnDeleteAllTracks->setEnabled(true);
        btnStartExport->setEnabled(true);

        progressBar->setVisible(false);
        progressBar->setValue(0);

        QString msg = QString("Done!\n\nLiked: %1\nNot found / failed: %2")
                          .arg(likedSuccessCount)
                          .arg(likedFailCount);
        if (ok) {
            msg += QString("\n\nLog saved: %1\n(You can open it via 'Recent actions'.)")
            .arg(QFileInfo(filePath).fileName());
        } else {
            msg += "\n\nWarning: could not save action log file.";
        }
        QMessageBox::information(this, "Done", msg);
        return;
    }

    searchTrack(trackList[currentTrackIndex]);
}

void MainWindow::searchTrack(const Track &t) {
    QUrl url("https://api.spotify.com/v1/search");
    QUrlQuery query;
    query.addQueryItem("q", QString("artist:%1 track:%2").arg(t.artist, t.title));
    query.addQueryItem("type", "track");
    query.addQueryItem("limit", "1");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply, t]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray items = doc.object()["tracks"].toObject()["items"].toArray();

            if (!items.isEmpty()) {
                QString trackId = items[0].toObject()["id"].toString();
                pendingTrackIds.append(QString("spotify:track:%1").arg(trackId));
                pendingTrackLines.append(QString("\"%1\" - \"%2\"").arg(t.artist, t.title));
            } else {
                qDebug() << "Not found:" << t.title;
                likedFailCount++;
                likedTrackLines.append(makeFailedLikeLine(t));
            }
        } else {
            likedFailCount++;
            likedTrackLines.append(makeFailedLikeLine(t));
        }
        reply->deleteLater();
        currentTrackIndex++;

        progressBar->setValue(currentTrackIndex);

        if (pendingTrackIds.size() >= 50) {
            flushPendingLikes();
        } else {
            QTimer::singleShot(100, this, &MainWindow::processNextTrack);
        }
    });
}

void MainWindow::flushPendingLikes() {
    if (pendingTrackIds.isEmpty()) {
        processNextTrack();
        return;
    }

    QUrl url("https://api.spotify.com/v1/me/library");
    QUrlQuery q;
    q.addQueryItem("uris", pendingTrackIds.join(","));
    url.setQuery(q);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->put(request, QByteArray());

    QStringList currentIds = pendingTrackIds;
    QStringList currentLines = pendingTrackLines;
    pendingTrackIds.clear();
    pendingTrackLines.clear();

    connect(reply, &QNetworkReply::finished, [this, reply, currentIds, currentLines]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status == 429) {
            const int waitMs = retryAfterMs(reply);
            pendingTrackIds = currentIds + pendingTrackIds;
            pendingTrackLines = currentLines + pendingTrackLines;
            reply->deleteLater();
            QTimer::singleShot(waitMs, this, &MainWindow::flushPendingLikes);
            return;
        }

        if (reply->error() == QNetworkReply::NoError && status == 200) {
            qDebug() << "Success! Batch of" << currentIds.size() << "tracks liked.";
            likedTrackLines.append(currentLines);
            likedSuccessCount += currentIds.size();
        } else {
            qDebug() << "Batch API Error:" << reply->errorString();
            likedFailCount += currentIds.size();
            for (const QString &line : currentLines) {
                likedTrackLines.append(markFailedLine(line));
            }
        }

        reply->deleteLater();
        QTimer::singleShot(200, this, &MainWindow::processNextTrack);
    });
}

QString MainWindow::makeActionFileBaseName(const QString &prefix) const {
    const QDateTime now = QDateTime::currentDateTime();
    const QString timePart = now.toString("HH-mm");
    const QString datePart = now.toString("MM.dd.yyyy");
    return QString("%1 %2 %3").arg(prefix, timePart, datePart);
}

bool MainWindow::saveActionFile(const QString &baseName, const QStringList &lines, QString *outFilePath) {
    if (actionsDir.isEmpty()) {
        return false;
    }

    QDir dir(actionsDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return false;
        }
    }

    QString fileName = baseName + ".txt";
    QString fullPath = dir.filePath(fileName);
    int i = 2;
    while (QFile::exists(fullPath)) {
        fileName = QString("%1 (%2).txt").arg(baseName).arg(i);
        fullPath = dir.filePath(fileName);
        ++i;
    }

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    for (const QString &line : lines) {
        out << line << "\n";
    }
    file.close();

    if (outFilePath) {
        *outFilePath = fullPath;
    }
    return true;
}

void MainWindow::onRecentActionsClicked() {
    showRecentActionsDialog();
}

void MainWindow::showRecentActionsDialog() {
    QDir dir(actionsDir);
    if (!dir.exists()) {
        QMessageBox::information(this, "Recent actions", "No actions yet.");
        return;
    }

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.txt",
                                            QDir::Files | QDir::NoDotAndDotDot,
                                            QDir::Time);
    if (files.isEmpty()) {
        QMessageBox::information(this, "Recent actions", "No actions yet.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Recent actions");
    dlg.resize(420, 360);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QLabel *info = new QLabel("Click a file to save it to your computer.", &dlg);
    layout->addWidget(info);

    QListWidget *list = new QListWidget(&dlg);
    for (const QFileInfo &fi : files) {
        QListWidgetItem *item = new QListWidgetItem(fi.fileName(), list);
        item->setData(Qt::UserRole, fi.absoluteFilePath());
    }
    layout->addWidget(list);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    connect(list, &QListWidget::itemClicked, this, [this, &dlg](QListWidgetItem *item) {
        const QString srcPath = item->data(Qt::UserRole).toString();
        const QFileInfo srcInfo(srcPath);

        const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        const QString suggested = QDir(downloads).filePath(srcInfo.fileName());

        const QString dstPath = QFileDialog::getSaveFileName(&dlg,
                                                             "Save action file",
                                                             suggested,
                                                             "Text Files (*.txt)");
        if (dstPath.isEmpty()) {
            return;
        }

        if (QFile::exists(dstPath)) {
            const auto res = QMessageBox::question(&dlg,
                                                   "Overwrite?",
                                                   "File already exists. Overwrite?",
                                                   QMessageBox::Yes | QMessageBox::Cancel,
                                                   QMessageBox::Cancel);
            if (res != QMessageBox::Yes) {
                return;
            }
            QFile::remove(dstPath);
        }

        if (!QFile::copy(srcPath, dstPath)) {
            QMessageBox::critical(&dlg, "Error", "Could not save file.");
            return;
        }

        QMessageBox::information(&dlg, "Saved", "File saved successfully.");
    });

    dlg.exec();
}

int MainWindow::retryAfterMs(QNetworkReply *reply) const {
    bool ok = false;
    const int sec = reply->rawHeader("Retry-After").toInt(&ok);
    if (ok && sec > 0) {
        return sec * 1000;
    }
    return 1000;
}


void MainWindow::onDeleteAllTracksClicked() {
    if (accessToken.isEmpty()) {
        QMessageBox::warning(this, "Auth Error", "Please connect to Spotify first!");
        return;
    }

    if (deleteInProgress) {
        QMessageBox::information(this, "Delete all tracks", "Delete operation is already running.");
        return;
    }

    if (!btnLikeTracks->isEnabled() && btnLikeTracks->text() == "Processing...") {
        QMessageBox::warning(this, "Busy", "Like operation is in progress. Please wait until it finishes.");
        return;
    }

    fetchSavedTracksTotalAndConfirm();
}

void MainWindow::fetchSavedTracksTotalAndConfirm() {
    QUrl url("https://api.spotify.com/v1/me/tracks");
    QUrlQuery q;
    q.addQueryItem("limit", "1");
    q.addQueryItem("offset", "0");
    url.setQuery(q);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();

        if (status == 429) {
            const int waitMs = retryAfterMs(reply);
            reply->deleteLater();
            QTimer::singleShot(waitMs, this, &MainWindow::fetchSavedTracksTotalAndConfirm);
            return;
        }

        if (reply->error() != QNetworkReply::NoError || status != 200) {
            const QString err = QString("Failed to read saved tracks. HTTP %1\n%2")
            .arg(status)
                .arg(QString::fromUtf8(body));
            reply->deleteLater();
            QMessageBox::critical(this, "Error", err);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        const int total = doc.object().value("total").toInt();
        reply->deleteLater();

        if (total <= 0) {
            QMessageBox::information(this, "Delete all tracks", "Your library is already empty.");
            return;
        }

        const auto res = QMessageBox::warning(
            this,
            "Delete all tracks",
            QString("This will remove %1 tracks from your Liked Songs.\n\nDo you want to continue?").arg(total),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel
            );

        if (res != QMessageBox::Yes) {
            return;
        }

        deleteInProgress = true;
        deleteTotal = total;
        deletedCount = 0;
        deletedTrackLines.clear();

        btnDeleteAllTracks->setEnabled(false);
        btnLikeTracks->setEnabled(false);
        btnStartExport->setEnabled(false);
        btnDeleteAllTracks->setText(QString("Deleting... 0/%1").arg(deleteTotal));

        deleteNextSavedTracksPage();
    });
}

void MainWindow::deleteNextSavedTracksPage() {
    if (!deleteInProgress) {
        return;
    }

    QUrl url("https://api.spotify.com/v1/me/tracks");
    QUrlQuery q;
    q.addQueryItem("limit", "40");
    q.addQueryItem("offset", "0");
    url.setQuery(q);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();

        if (status == 429) {
            const int waitMs = retryAfterMs(reply);
            reply->deleteLater();
            QTimer::singleShot(waitMs, this, &MainWindow::deleteNextSavedTracksPage);
            return;
        }

        if (reply->error() != QNetworkReply::NoError || status != 200) {
            const QString err = QString("Failed to load saved tracks page. HTTP %1\n%2")
            .arg(status)
                .arg(QString::fromUtf8(body));
            reply->deleteLater();
            finishDeleteFlow(false, err);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        const QJsonArray items = doc.object().value("items").toArray();
        reply->deleteLater();

        if (items.isEmpty()) {
            finishDeleteFlow(true);
            return;
        }

        QStringList uris;
        QStringList lines;

        for (const QJsonValue &v : items) {
            const QJsonObject itemObj = v.toObject();
            const QJsonObject trackObj = itemObj.value("track").toObject();

            const QString uri = trackObj.value("uri").toString();
            const QString name = trackObj.value("name").toString();

            QStringList artistNames;
            const QJsonArray artistsArr = trackObj.value("artists").toArray();
            for (const QJsonValue &a : artistsArr) {
                const QString an = a.toObject().value("name").toString();
                if (!an.isEmpty()) artistNames << an;
            }
            const QString artist = artistNames.join(", ");

            if (!uri.isEmpty()) {
                uris << uri;
                lines << QString("\"%1\" - \"%2\"").arg(artist, name);
            }
        }

        if (uris.isEmpty()) {
            finishDeleteFlow(true);
            return;
        }

        deleteLibraryUris(uris, lines);
    });
}

void MainWindow::deleteLibraryUris(const QStringList &uris, const QStringList &lines) {
    if (!deleteInProgress) {
        return;
    }

    QUrl url("https://api.spotify.com/v1/me/library");
    QUrlQuery q;
    q.addQueryItem("uris", uris.join(","));
    url.setQuery(q);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    QNetworkReply *reply = networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, [this, reply, uris, lines]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();

        if (status == 429) {
            const int waitMs = retryAfterMs(reply);
            reply->deleteLater();
            QTimer::singleShot(waitMs, this, [this, uris, lines]() { deleteLibraryUris(uris, lines); });
            return;
        }

        if (reply->error() != QNetworkReply::NoError || status != 200) {
            const QString err = QString("Failed to delete from library. HTTP %1\n%2")
            .arg(status)
                .arg(QString::fromUtf8(body));
            reply->deleteLater();
            finishDeleteFlow(false, err);
            return;
        }

        deletedTrackLines += lines;
        deletedCount += uris.size();
        btnDeleteAllTracks->setText(QString("Deleting... %1/%2").arg(deletedCount).arg(deleteTotal));

        reply->deleteLater();
        QTimer::singleShot(200, this, &MainWindow::deleteNextSavedTracksPage);
    });
}

void MainWindow::finishDeleteFlow(bool success, const QString &errorMessage) {
    deleteInProgress = false;

    btnDeleteAllTracks->setText("Delete all tracks");
    btnDeleteAllTracks->setEnabled(true);
    btnLikeTracks->setEnabled(true);
    btnStartExport->setEnabled(true);

    if (!success) {
        QMessageBox::critical(this, "Delete all tracks", errorMessage.isEmpty() ? "Operation failed." : errorMessage);
        return;
    }

    QString filePath;
    const QString baseName = makeActionFileBaseName("Deleted");
    const bool ok = saveActionFile(baseName, deletedTrackLines, &filePath);

    QString msg = QString("Deleted %1 tracks.").arg(deletedCount);
    if (ok) {
        msg += QString("\n\nLog saved: %1\n(You can open it via 'Recent actions'.)")
        .arg(QFileInfo(filePath).fileName());
        msg += "\n\nNote: ':' is not allowed in Windows file names, so time is saved as HH-mm.";
    } else {
        msg += "\n\nWarning: could not save action log file.";
    }

    QMessageBox::information(this, "Delete all tracks", msg);
}

MainWindow::~MainWindow() {}
