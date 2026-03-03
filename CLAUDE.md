# TVTest Android Version

## プロジェクト概要
Windows版TVTestをQt for Androidで移植する視聴専用アプリケーション

### 現在のステータス
- [x] プロジェクト構造調査完了
- [x] 既存機能分析完了
- [x] Qt for Android技術選定完了
- [x] Android Emulator採用決定
- [x] JDK 17 インストール完了
- [x] Android Studio セットアップ完了
- [ ] Qt Creator + Android統合
- [ ] プロジェクト初期化
- [ ] コア機能移植
- [ ] UI実装
- [ ] テスト・デバッグ

## 実装計画

### フェーズ1: 基盤準備
1. Qt for Androidプロジェクト作成
2. CMakeビルド設定
3. 既存LibISDB統合
4. ログシステム実装

### フェーズ2: コア機能移植
1. TSProcessor → Qt統合
2. MPEG-2パーサー → Android対応  
3. EPGエンジン → Qt SQL統合
4. チャンネル管理 → QSettings

### フェーズ3: UI/UX開発 (Qt Designer)
1. メイン画面 (動画プレイヤー) - mainwindow.ui
2. チャンネルリスト画面 - channellist.ui
3. EPG表示画面 - epgdialog.ui
4. 設定画面 - settings.ui

### 除外機能
- 録画機能 (RecorderFilter)
- キャプチャ機能 (CaptureOptions)
- 録画スケジューリング

## 技術スタック

### 開発環境
- **Qt Creator 11.x**
- **Qt 6.6+ for Android**
- **Android NDK r25+**
- **CMake 3.21+**

### コア技術
- **C++17** (既存コード活用)
- **Qt Widgets** (UI描画)
- **Qt Designer** (.uiファイル)
- **Qt Multimedia** (メディア再生)
- **Qt Network** (通信処理)
- **Qt SQL** (データベース)

### 依存ライブラリ
- **LibISDB** (MPEG-2 TS処理)
- **Qt Widgets** (UIコンポーネント)
- **Qt Designer** (UI設計ツール)

## ディレクトリ構造

```
android_tvtest/
├── src/                    # C++ソースコード
│   ├── core/              # コアエンジン移植
│   ├── network/           # ネットワーク処理
│   ├── database/          # EPG・設定データ
│   └── utils/             # ユーティリティ
├── ui/                    # UIファイル
│   ├── mainwindow.ui     # メイン画面
│   ├── channellist.ui    # チャンネルリスト
│   ├── epgdialog.ui      # EPG表示
│   └── settings.ui       # 設定画面
├── resources/            # リソースファイル
├── libs/                 # 外部ライブラリ
│   └── LibISDB/         # 移植版
└── docs/                # ドキュメント
```

## ビルド設定

### ビルド出力先
```bash
# デバッグビルド
build/debug/android/

# リリースビルド  
build/release/android/

# APKファイル
dist/android/tvtest-android-v*.apk
```

### ビルドコマンド
```bash
# デバッグビルド
qmake CONFIG+=debug android
make

# リリースビルド
qmake CONFIG+=release android
make

# APK生成
androiddeployqt --input android-build/android-tvtest-deployment-settings.json --output android-build --android-platform android-33 --jdk $JAVA_HOME --gradle
```

## ログ設定

### ログファイル保存先
```
Android内部ストレージ:
/storage/emulated/0/Android/data/com.tvtest.android/files/logs/

ログファイル:
├── tvtest_debug.log       # デバッグログ
├── tvtest_error.log       # エラーログ  
├── network.log           # ネットワーク通信ログ
├── media.log             # メディア処理ログ
└── crash.log             # クラッシュログ
```

### ログレベル
```cpp
enum LogLevel {
    LOG_TRACE = 0,    // 詳細トレース
    LOG_DEBUG = 1,    // デバッグ情報
    LOG_INFO  = 2,    // 一般情報
    LOG_WARN  = 3,    // 警告
    LOG_ERROR = 4,    // エラー
    LOG_FATAL = 5     // 致命的エラー
};
```

### デバッグ設定
```cpp
// デバッグビルドでは全レベル出力
#ifdef DEBUG
    #define LOG_LEVEL LOG_TRACE
#else
    #define LOG_LEVEL LOG_INFO
#endif

// ログファイルサイズ制限: 10MB
#define MAX_LOG_FILE_SIZE (10 * 1024 * 1024)

// ローテーション: 5世代保持
#define LOG_FILE_ROTATION 5
```

## 実行環境 (Android Emulator)

### AVD設定 (推奨)
```
TVTest専用AVD:
├── Device: Pixel 7 Pro (6.7", 1440x3120)
├── API Level: 33 (Android 13.0)
├── Target: Google APIs
├── RAM: 4096 MB
├── Storage: 8 GB
└── Graphics: Hardware - GLES 2.0
```

### 開発ワークフロー
```bash
1. emulator -avd TVTest_AVD -gpu host -memory 4096
2. Qt Creator でビルド・自動インストール
3. adb logcat でログ確認
4. デバッグ → 修正 → 再ビルド
```

### 必要システム要件
- **RAM**: 8GB以上推奨
- **CPU**: Intel VT-x/AMD-V対応
- **GPU**: 統合GPU以上
- **Storage**: SSD推奨

## 開発メモ

### 現在の課題
- [ ] LibISDBのAndroid移植対応
- [ ] DirectShow依存の除去方法
- [ ] メディア再生の最適化

### 検討事項
- Qt MultimediaのHLSストリーミング対応
- Android権限管理 (INTERNET, WRITE_EXTERNAL_STORAGE)
- 省電力モード対応

### 参考リンク
- [Qt for Android Documentation](https://doc.qt.io/qt-6/android.html)
- [Qt Multimedia](https://doc.qt.io/qt-6/qtmultimedia-index.html)
- [Android NDK Guide](https://developer.android.com/ndk)

---
**最終更新**: 2026-03-02  
**作成者**: Claude Code