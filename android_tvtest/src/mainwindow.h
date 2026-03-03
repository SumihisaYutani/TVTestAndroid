#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMenuBar;
class QStatusBar;
QT_END_NAMESPACE

class CoreEngine;
class NetworkManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void playChannel();
    void stopPlayback();
    void showChannelList();
    void showEPG();
    void showSettings();
    void onChannelSelected();
    void updateStatus();

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void connectSignals();

    // UI Components
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QVideoWidget *m_videoWidget;
    QListWidget *m_channelListWidget;
    
    // Control buttons
    QPushButton *m_playButton;
    QPushButton *m_stopButton;
    QPushButton *m_channelButton;
    QPushButton *m_epgButton;
    QPushButton *m_settingsButton;
    
    // Status
    QLabel *m_statusLabel;
    QLabel *m_channelLabel;
    QTimer *m_statusTimer;
    
    // Media
    QMediaPlayer *m_mediaPlayer;
    
    // Core components
    CoreEngine *m_coreEngine;
    NetworkManager *m_networkManager;
    
    // State
    bool m_isPlaying;
    QString m_currentChannel;
};

#endif // MAINWINDOW_H