#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressBar>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onModeChanged();
    void onChooseFileClicked();
    void onNewConnection();
    void onStartExportClicked();
    void onRemoveFileClicked();

    void onLikeTracksClicked();
    void onDeleteAllTracksClicked();
    void onRecentActionsClicked();

    void processNextTrack();             // like-flow step
    void deleteNextSavedTracksPage();    // delete-flow step

private:
    void setupUI();

    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QPushButton *btnConnect;
    QRadioButton *radioFile;
    QRadioButton *radioText;
    QPushButton *btnChooseFile;
    QLabel *lblFileName;
    QTextEdit *txtEditor;

    QTcpServer *server;
    QString clientId;
    QString redirectUri = "http://127.0.0.1:8888/callback";

    void exchangeCodeForToken(QString code);
    QString accessToken;
    QString clientSecret;

    struct Track {
        QString artist;
        QString title;
    };
    QList<Track> trackList;

    void parseLine(QString line);
    QPushButton *btnStartExport;
    QString fullFilePath;
    QPushButton *btnRemoveFile;

    QPushButton *btnLikeTracks;
    QProgressBar *progressBar;

    QPushButton *btnDeleteAllTracks;
    QPushButton *btnRecentActions;

    QString actionsDir;

    int currentTrackIndex = 0;
    QNetworkAccessManager *networkManager;

    void searchTrack(const Track &t);

    QString makeActionFileBaseName(const QString &prefix) const;
    bool saveActionFile(const QString &baseName, const QStringList &lines, QString *outFilePath = nullptr);
    void showRecentActionsDialog();

    QStringList likedTrackLines;
    int likedSuccessCount = 0;
    int likedFailCount = 0;

    bool deleteInProgress = false;
    int deleteTotal = 0;
    int deletedCount = 0;
    QStringList deletedTrackLines;

    void fetchSavedTracksTotalAndConfirm();
    void deleteLibraryUris(const QStringList &uris, const QStringList &lines);
    void finishDeleteFlow(bool success, const QString &errorMessage = QString());
    int retryAfterMs(QNetworkReply *reply) const;

    QStringList pendingTrackIds;
    QStringList pendingTrackLines;

    QString makeLikeLine(const Track &t) const; // "Artist" - "Song"
    QString makeFailedLikeLine(const Track &t) const; // "Artist" - "Song" - Failed
    QString markFailedLine(const QString &okLine) const;
    void flushPendingLikes();

};

#endif // MAINWINDOW_H
