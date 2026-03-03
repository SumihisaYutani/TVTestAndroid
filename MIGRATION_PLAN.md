# TVTest Android移植 - DirectShow依存除去計画

## 🎯 移植戦略概要

### フェーズ1: Qt Multimedia移行 (優先度: 高)
**対象**: Windows DirectShow → Qt Multimedia

#### 1.1 メディア再生エンジン移行
```cpp
// 移行前 (Windows DirectShow)
IGraphBuilder *m_pGraphBuilder;
IMediaControl *m_pMediaControl; 
IVideoWindow *m_pVideoWindow;

// 移行後 (Qt Multimedia)
QMediaPlayer *m_mediaPlayer;
QVideoWidget *m_videoWidget;
QAudioOutput *m_audioOutput;
```

#### 1.2 ストリーム処理移行
- **TSProcessor.cpp/h** → **Qt Multimedia + LibISDB統合**
- **DirectShowフィルター** → **Qt MediaObjectクラス継承**

### フェーズ2: LibISDB Android対応 (優先度: 高)
**対象**: src/LibISDB/ Windows固有部分の抽象化

#### 2.1 プラットフォーム抽象化
```cpp
// Windows固有 (除去対象)
src/LibISDB/LibISDB/Windows/
├── Viewer/ViewerEngine.hpp      → Qt統合版
├── Filters/BonDriverSourceFilter.hpp → NetworkSourceFilter
└── DirectShow依存部分            → Qt Multimedia置換

// クロスプラットフォーム (維持)
src/LibISDB/LibISDB/
├── Base/                        ✅ 
├── TS/                         ✅
├── EPG/                        ✅
└── MediaParsers/               ✅
```

#### 2.2 ネットワークストリーミング対応
```cpp
// BonDriver → NetworkManager
class NetworkManager : public QObject {
    QNetworkAccessManager *m_network;
    QTcpSocket *m_streamSocket;
    
    // HLS/DASH/UDP対応
    void openNetworkStream(const QString &url);
    void processTSPackets(const QByteArray &data);
};
```

### フェーズ3: Android固有最適化 (優先度: 中)

#### 3.1 UI/UX Android対応
```cpp
// タッチ操作対応
class AndroidMainWindow : public QMainWindow {
    void setupTouchGestures();
    void handleSwipeGesture(QSwipeGesture *gesture);
    void handlePinchGesture(QPinchGesture *gesture);
};

// 画面回転対応
void handleOrientationChange(Qt::ScreenOrientation orientation);
```

#### 3.2 Android権限管理
```xml
<!-- android/AndroidManifest.xml -->
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WAKE_LOCK" />
```

## 🔧 具体的実装手順

### Step 1: CoreEngine.cpp基本実装
```cpp
bool CoreEngine::startPlayback() {
    if (!m_mediaPlayer) return false;
    
    // DirectShow IMediaControl::Run() の代替
    m_mediaPlayer->play();
    setState(EngineState::Running);
    return true;
}

bool CoreEngine::openStream(const QString &url) {
    // BonDriver::OpenTuner() の代替
    if (url.startsWith("http")) {
        // ネットワークストリーム
        m_networkManager->openNetworkStream(url);
    } else {
        // ローカルファイル
        m_mediaPlayer->setSource(QUrl::fromLocalFile(url));
    }
    return true;
}
```

### Step 2: TSProcessor Qt統合
```cpp
// TSProcessor.h 移行版
class QtTSProcessor : public QObject {
    Q_OBJECT
    
private:
    // LibISDB TSPacketParser統合
    LibISDB::TSPacketParserFilter *m_tsParser;
    QThread *m_processingThread;
    
public slots:
    void processNetworkData(const QByteArray &tsData);
    
signals:
    void videoDataReady(const QByteArray &data);
    void audioDataReady(const QByteArray &data);
};
```

### Step 3: 録画機能除去
```cpp
// 除去対象 (Android版では非対応)
- RecorderFilter.cpp/h
- CaptureOptions.cpp/h  
- RecordManager.cpp/h

// 残す (視聴専用機能)
- ViewerEngine ✅
- EPGDatabaseFilter ✅
- ChannelManager ✅
```

## 📋 技術的課題と解決策

### 課題1: リアルタイムTS処理性能
**解決策**: QThreadプール + LibISDB最適化
```cpp
class TSProcessingThread : public QThread {
    void run() override {
        // LibISDB TSPacketParser
        // Android ARM最適化
    }
};
```

### 課題2: メモリ使用量制限
**解決策**: ストリーミングバッファ管理
```cpp
// Android向けバッファサイズ調整
#ifdef Q_OS_ANDROID
    #define TS_BUFFER_SIZE (1024 * 1024)      // 1MB
    #define MAX_BUFFERS    10
#else
    #define TS_BUFFER_SIZE (4 * 1024 * 1024)  // 4MB  
    #define MAX_BUFFERS    20
#endif
```

### 課題3: ネットワーク接続安定性
**解決策**: 自動再接続 + QoS制御
```cpp
class NetworkManager : public QObject {
private:
    QTimer *m_reconnectTimer;
    int m_reconnectAttempts;
    
private slots:
    void attemptReconnect();
    void handleNetworkError(QNetworkReply::NetworkError error);
};
```

## 🎯 完了判定基準

### フェーズ1完了条件
- [x] Qt Multimedia基本再生動作 ✅
- [ ] ネットワークストリーム取得
- [ ] TSPacket処理パイプライン
- [ ] EPG表示基本機能

### フェーズ2完了条件  
- [ ] LibISDB Windows部分抽象化
- [ ] TSProcessor Qt統合完了
- [ ] チャンネル切替動作
- [ ] 信号レベル監視

### フェーズ3完了条件
- [ ] Android UI最適化
- [ ] タッチジェスチャー対応
- [ ] バックグラウンド再生
- [ ] 省電力モード対応

---
**進捗更新**: 2026-03-03  
**次回レビュー**: フェーズ1完了後