# TVTest Android Version

## プロジェクト概要
Windows版TVTestをQt for Androidで移植する視聴専用アプリケーション。
現在はデスクトップ版（Windows Qt6）でBonDriverProxyEx経由のリアルタイムTSストリーミング再生を実装・改善中。

---

## 現在の実装状況（2026-03-22時点）

### 動作している機能
- [x] BonDriverProxyEx TCP接続（192.168.0.5:1192 / baruma.f5.si:1192）
- [x] サーバー選択プルダウン（QComboBox、編集可能）
- [x] GetTsStream ポーリングによるTSデータ受信
- [x] TSパケット同期（0x47検出）
- [x] TSStreamingServer（HTTP localhost:8080）経由でVLCに配信
- [x] VLC 3.0.23 による映像・音声再生（音声は正常）
- [x] ログファイル出力（build/Desktop_.../logs/tvtest_*.log）

### 未解決の問題
- [ ] 映像が「下半分だけ表示」「高速で2種類の画像が交互」→ 調査・対応中
  - 音声は正常、映像のみ乱れ
  - 1080iインターレース映像のフィールド表示問題と推定
  - `--vout=direct3d9` で drawable window query 3 エラーは解消
  - `libvlc_video_set_deinterlace("yadif2x")` を Playing イベント後に適用中
  - **次回**: ログの `🎬 VLCデコード映像サイズ` で実際の解像度を確認して原因を絞り込む

---

## アーキテクチャ

### データフロー
```
BonDriverProxyEx サーバー (192.168.0.5:1192)
    ↓ TCP (BonDriverNetwork)
    ↓ GetTsStream 100ms ポーリング
    ↓ TSパケット(188byte)抽出・同期
    ↓ emit tsDataReceived(QByteArray)
TSStreamingServer (HTTP localhost:8080)
    ↓ HTTP/1.1 200 OK + raw TS stream
VLC 3.0.23 (libvlc embedded)
    ↓ direct3d9 レンダリング
映像ウィジェット (QWidget HWND)
```

### ファイル構成
```
TVTestAndroid/
├── CMakeLists.txt                          # ビルド設定（VLC 3.0.23 SDK参照）
├── vlc-3.0.23/                             # VLC SDK（gitignore対象）
├── src/
│   ├── network/
│   │   ├── BonDriverNetwork.cpp/h          # BonDriverProxyEx プロトコル実装
│   │   ├── TSStreamingServer.cpp/h         # HTTP TSストリーミングサーバー
│   │   └── VLCStreamingPlayer.cpp/h        # VLC制御・映像出力
│   ├── ui/
│   │   └── MainWindow.cpp/h                # メインウィンドウ・UI
│   └── utils/
│       └── Logger.cpp/h                    # ログシステム
└── resources/
    └── channels.json                       # チャンネル設定
```

---

## BonDriverProxyEx プロトコル（解析済み）

### パケットフォーマット
```
[0xFF][cmd][reserved: 2bytes][size: 4bytes big-endian][payload...]
```

### GetTsStream レスポンスの処理
- ヘッダー `[ff 08 00 00 00 02 f0 08]` の bytes[2..5] を LE 読みすると誤値になる
- **修正済み**: GetTsStream (cmd=0x08) は8バイトヘッダーだけ削除、後続TSデータはそのまま処理
- PurgeTsStream は通常ストリーミング中に呼ぶとサーバーバッファが破棄されTSデータが欠落するため除外済み

### コマンドサイクル（現在）
- GetTsStream: 毎100ms（メイン）
- GetSignalLevel: 100回に1回（10秒間隔）
- PurgeTsStream: **使用停止**（欠落の原因のため）

---

## VLC設定（現在）

### VLCインスタンス引数
```cpp
"--intf", "dummy"
"--no-video-title-show"
"--live-caching=300"
"--file-caching=300"
"--network-caching=300"
"--clock-jitter=0"
"--clock-synchro=0"
"--no-skip-frames"
"--vout=direct3d9"    // drawable window query 3 エラー回避
"--verbose=1"
```

### メディアオプション
```cpp
":demux=ts"
":ts-es-id-pid"
":no-ts-trust-pcr"
":ts-seek-percent=false"
":live-caching=300"
":network-caching=300"
":input-repeat=999999"
":start-time=0"
```

### 再生開始後に適用（Playing イベント後・メインスレッド）
```cpp
libvlc_video_set_deinterlace(m_vlcPlayer, "yadif2x");  // 1080i対応
libvlc_video_set_scale(m_vlcPlayer, 0);                // ウィジェット全体に表示
```

---

## ビルド方法

### 環境
- Qt 6.10.1 MinGW 64bit
- VLC 3.0.23 SDK: `TVTestAndroid/vlc-3.0.23/sdk/`
- ビルドディレクトリ: `TVTestAndroid/build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug/`

### ビルドコマンド
```bash
cd TVTestAndroid/build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug
C:\Qt\Tools\Ninja\ninja.exe TVTestAndroid
```
※ アプリ起動中はexeがロックされるため先に終了すること

### 実行
```
build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug/TVTestAndroid.exe
```

---

## ログ設定

### ログファイル保存先
```
build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug/logs/tvtest_YYYYMMDD_HHMMSS.log
```

### デバッグのポイント
- `⚠️ 異常なレスポンスサイズ` → GetTsStream ヘッダー解析エラー（修正済み）
- `📡 GetTsStreamヘッダー #N` → ヘッダー正常処理の確認
- `🎬 VLCデコード映像サイズ` → 映像解像度確認（次回デバッグで重要）
- `🖥️ 映像ウィジェットサイズ` → ウィジェット vs 映像サイズの比較

---

## 今後の課題

1. **映像表示問題の解決**（最優先）
   - `🎬 VLCデコード映像サイズ` ログで実際の解像度を確認
   - 1440x540（1フィールド）ならインターレース処理問題
   - 1440x1080 でウィジェットより大きければスケーリング問題
   - 0x0 なら Playing イベント時点で映像未初期化

2. **Android移植**（将来）
   - VLCをAndroid対応版に差し替え
   - Qt for Android ビルド設定

---
**最終更新**: 2026-03-22
**作成者**: Claude Code
